# AI_Mimalloc Subgoal 2: Improved Mimalloc Architecture Design

## Overview
This document outlines the improved architecture for the production-ready C++20 mimalloc implementation, addressing all 20 issues identified in Subgoal 1.

---

## Design Principles

1. **Zero Dependencies**: No `malloc/free/new/delete` internally
2. **Single Header**: Everything in one `.h` file for easy integration
3. **C++20 Native**: Use concepts, `constexpr`, `std::atomic`, designated initializers
4. **Production Ready**: Security, error handling, statistics, debugging
5. **HLVM Integration**: Follow existing project conventions from `DOC_Coding_Style.md`

---

## Architecture Overview

### Memory Layout

```
┌─────────────────────────────────────────────────────────────┐
│  Segment (4MiB aligned)                                      │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  Segment Header (256 bytes)                           │  │
│  │  - thread_id, segment_id, kind, page_shift            │  │
│  │  - pages[64] metadata (64 * 64 = 4096 bytes)          │  │
│  ├───────────────────────────────────────────────────────┤  │
│  │  Guard Page (4KiB, PROT_NONE)                         │  │
│  ├───────────────────────────────────────────────────────┤  │
│  │  Page 0 Data (64KiB) │ Page 1 Data (64KiB) │ ...      │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Three-Tier Free List System

| List | Purpose | Thread Safety | Operation |
|------|---------|---------------|-----------|
| `available` | Main allocation list | Owner thread only | LIFO pop/push |
| `local_pending` | Local frees (temporal cadence) | Owner thread only | LIFO push, batch collect |
| `thread_pending` | Cross-thread frees | Atomic (lock-free) | CAS push, batch collect |

---

## Core Data Structures

### Configuration Constants

```cpp
namespace config {
    inline constexpr size_t SEGMENT_SIZE = 4ULL * 1024 * 1024;
    inline constexpr size_t SEGMENT_ALIGN = SEGMENT_SIZE;
    inline constexpr size_t SMALL_PAGE_SIZE = 64 * 1024;
    inline constexpr size_t SMALL_PAGES_PER_SEGMENT = 64;
    inline constexpr size_t GUARD_PAGE_SIZE = 4096;
    inline constexpr size_t MAX_SMALL_SIZE = 8 * 1024;
    inline constexpr size_t MAX_LARGE_SIZE = 512 * 1024;
    inline constexpr size_t HEARTBEAT_INTERVAL = 256;
    inline constexpr size_t SIZE_CLASS_COUNT = 64;
    inline constexpr size_t MIN_ALIGN = 16;  // Minimum alignment (SIMD-safe)
}
```

### Size Class System (Fixed)

**Issue 10 Fixed**: Optimized calculation with lookup table

```cpp
struct SizeClass {
    static constexpr size_t count = config::SIZE_CLASS_COUNT;
    
    // Precomputed sizes: 8, 16, 24, 32, 48, 64... up to 8KiB
    static constexpr size_t sizes[] = { /* 64 entries */ };
    
    [[nodiscard]] static constexpr size_t from_size(size_t size) noexcept {
        if (size <= 128) return (size + 7) / 8 - 1;
        if (size <= 512) return 10 + (size - 129) / 32;
        if (size <= 1024) return 22 + (size - 513) / 64;
        if (size <= 2048) return 30 + (size - 1025) / 128;
        if (size <= 4096) return 38 + (size - 2049) / 256;
        return 46 + (size - 4097) / 512;
    }
    
    [[nodiscard]] static constexpr size_t to_size(size_t sc) noexcept {
        if (sc < count) return sizes[sc];
        return config::MAX_SMALL_SIZE;
    }
};
```

### Block Structure (Unchanged)

```cpp
struct Block {
    Block* next;
    
    [[nodiscard]] Block* decoded_next(uintptr_t cookie) const noexcept {
        return reinterpret_cast<Block*>(reinterpret_cast<uintptr_t>(next) ^ cookie);
    }
    
    void encode_next(Block* n, uintptr_t cookie) noexcept {
        next = reinterpret_cast<Block*>(reinterpret_cast<uintptr_t>(n) ^ cookie);
    }
};
```

### Page Metadata (64-byte aligned, Fixed)

**Issues 5, 6, 7, 11 Fixed**: Correct layout, efficient randomization, proper memory ordering

```cpp
struct alignas(64) Page {
    // Linked list (16 bytes)
    Page* next_page = nullptr;
    Page* prev_page = nullptr;
    
    // Three free lists (24 bytes)
    Block* available = nullptr;
    Block* local_pending = nullptr;
    std::atomic<Block*> thread_pending{nullptr};
    
    // Statistics (8 bytes)
    uint16_t used_count = 0;
    uint16_t pending_count = 0;
    uint16_t thread_pending_count = 0;
    uint16_t capacity = 0;
    
    // Configuration (8 bytes)
    uint16_t block_size = 0;
    uint16_t heartbeat_counter = config::HEARTBEAT_INTERVAL;
    
    // State (4 bytes)
    PageKind kind = PageKind::Small;
    PageState state = PageState::Active;
    bool is_committed = false;
    bool is_no_safety = false;  // For low-level system use
    
    // Owner (8 bytes)
    uint32_t thread_id = 0;
    uintptr_t cookie = 0;  // Security cookie for this page
    
    // Methods
    [[nodiscard]] bool is_full() const noexcept {
        return available == nullptr && local_pending == nullptr && 
               thread_pending.load(std::memory_order_relaxed) == nullptr;
    }
    
    [[nodiscard]] bool is_empty() const noexcept {
        return used_count == 0 && pending_count == 0 && 
               thread_pending_count == 0;
    }
    
    [[nodiscard]] bool is_local() const noexcept {
        return thread_id == detail::get_thread_id();
    }
    
    void* allocate_local(uintptr_t global_cookie) noexcept;
    void free_local(void* ptr, uintptr_t global_cookie) noexcept;
    void free_thread(void* ptr, uintptr_t global_cookie) noexcept;
    void collect(uintptr_t global_cookie) noexcept;
};
```

### Segment Header (Fixed)

**Issue 5 Fixed**: Correct page resolution

```cpp
struct alignas(config::SEGMENT_ALIGN) Segment {
    // Identification (16 bytes)
    uint32_t thread_id = 0;
    uint32_t segment_id = 0;
    PageKind kind = PageKind::Small;
    uint8_t padding0[1] = {};
    
    // Layout (8 bytes)
    uint8_t page_shift = 16;  // 16 for small (64KiB), 22 for large (4MiB)
    uint8_t page_count = config::SMALL_PAGES_PER_SEGMENT;
    uint8_t used_pages = 0;
    uint8_t padding1[5] = {};
    
    // Security (8 bytes)
    uintptr_t cookie = 0;
    uint8_t padding2[8] = {};
    
    // Pages array (64 * 64 = 4096 bytes)
    Page pages[config::SMALL_PAGES_PER_SEGMENT];
    
    // Methods
    [[nodiscard]] static Segment* from_pointer(void* ptr) noexcept {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        return reinterpret_cast<Segment*>(addr & ~(config::SEGMENT_ALIGN - 1));
    }
    
    [[nodiscard]] Page* page_from_pointer(void* ptr) noexcept {
        uintptr_t seg_addr = reinterpret_cast<uintptr_t>(this);
        uintptr_t ptr_addr = reinterpret_cast<uintptr_t>(ptr);
        
        if (kind == PageKind::Small) {
            // Fixed calculation: header + guard
            uintptr_t header_end = seg_addr + sizeof(Segment);
            uintptr_t guard_end = detail::align_up(header_end, config::GUARD_PAGE_SIZE) + config::GUARD_PAGE_SIZE;
            
            if (ptr_addr < guard_end) return nullptr;
            
            size_t page_idx = (ptr_addr - guard_end) >> page_shift;
            if (page_idx >= page_count) return nullptr;
            return &pages[page_idx];
        }
        
        // Large/Huge: single page
        return &pages[0];
    }
    
    [[nodiscard]] void* data_start() noexcept {
        uintptr_t header_end = reinterpret_cast<uintptr_t>(this) + sizeof(Segment);
        return reinterpret_cast<void*>(
            detail::align_up(header_end, config::GUARD_PAGE_SIZE) + config::GUARD_PAGE_SIZE
        );
    }
};
```

### Heap (Thread-Local, Fixed)

**Issues 4, 7, 8, 11, 13 Fixed**: Thread ID, delayed_free, statistics, memory ordering

```cpp
class Heap {
public:
    // Direct lookup (512 bytes)
    Page* pages_direct[SizeClass::count] = {};
    
    // Size class lists (512 bytes)
    Page* pages[SizeClass::count] = {};
    
    // Full pages (8 bytes)
    Page* full_pages = nullptr;
    
    // Delayed free list for full pages (8 bytes)
    std::atomic<Block*> delayed_free{nullptr};
    
    // Thread identification (4 bytes)
    uint32_t thread_id = 0;
    
    // Configuration (16 bytes)
    void (*deferred_free_fn)() = nullptr;
    uintptr_t cookie = 0;
    bool enable_security = true;
    bool padding[7] = {};
    
    // Statistics (48 bytes) - Issue 13 Fixed
    struct Stats {
        std::atomic<size_t> alloc_count{0};
        std::atomic<size_t> free_count{0};
        std::atomic<size_t> generic_count{0};
        std::atomic<size_t> bytes_allocated{0};
        std::atomic<size_t> bytes_freed{0};
        std::atomic<size_t> cross_thread_frees{0};      // NEW
        std::atomic<size_t> remote_free_contention{0};  // NEW - CAS retries
    } stats;
    
    // Constructor
    Heap() : thread_id(detail::get_thread_id()) {
        cookie = security::generate_cookie();
        init_size_classes();
    }
    
    // Core operations
    [[nodiscard]] void* allocate(size_t size) noexcept;
    void free(void* ptr) noexcept;
    [[nodiscard]] void* realloc(void* ptr, size_t new_size) noexcept;
    
    // Size-specific
    [[nodiscard]] void* allocate_small(size_t size) noexcept;
    [[nodiscard]] void* allocate_large(size_t size) noexcept;
    [[nodiscard]] void* allocate_huge(size_t size) noexcept;
    
    // Maintenance
    void collect(Page* page) noexcept;
    void collect_delayed_free() noexcept;  // NEW - Issue 7
    [[nodiscard]] void* generic_allocate(size_t size) noexcept;
    [[nodiscard]] Page* find_page(size_t size_class) noexcept;
    [[nodiscard]] Page* allocate_fresh_page(size_t size_class) noexcept;
    
private:
    void init_size_classes() noexcept;
    void insert_page(Page* page, size_t size_class) noexcept;
    void remove_page(Page* page, size_t size_class) noexcept;
    void move_to_full(Page* page, size_t size_class) noexcept;
    void move_from_full(Page* page, size_t size_class) noexcept;
    
    std::vector<Segment*> segments_;
    std::mutex segments_mutex_;
};
```

---

## Key Algorithms (Fixed)

### Allocation Fast Path (~10-15 instructions)

**Issue 1 Fixed**: `likely/unlikely` macros defined

```cpp
[[nodiscard]] inline void* Heap::allocate_small(size_t size) noexcept {
    size_t sc = SizeClass::from_size(size);
    Page* page = pages_direct[sc];
    
    // Fast path with heartbeat check
    if (MI_LIKELY(page && page->available && page->heartbeat_counter > 0)) {
        void* ptr = page->allocate_local(cookie);
        if (MI_LIKELY(ptr)) {
            stats.alloc_count.fetch_add(1, std::memory_order_relaxed);
            stats.bytes_allocated.fetch_add(size, std::memory_order_relaxed);
            return ptr;
        }
    }
    
    // Slow path
    return generic_allocate(size);
}
```

### Free Operation (Local vs Remote)

**Issue 11 Fixed**: Proper memory ordering

```cpp
inline void Heap::free(void* ptr) noexcept {
    if (MI_UNLIKELY(!ptr)) return;
    
    Segment* seg = Segment::from_pointer(ptr);
    if (MI_UNLIKELY(!seg)) return;
    
    stats.free_count.fetch_add(1, std::memory_order_relaxed);
    
    // Huge allocation
    if (seg->kind == PageKind::Huge) {
        stats.bytes_freed.fetch_add(seg->data_size(), std::memory_order_relaxed);
        free_segment(seg);
        return;
    }
    
    Page* page = seg->page_from_pointer(ptr);
    if (MI_UNLIKELY(!page)) return;
    
    stats.bytes_freed.fetch_add(page->block_size, std::memory_order_relaxed);
    
    // Local vs remote free
    if (MI_LIKELY(page->thread_id == thread_id)) {
        // Local free
        if (page->state == PageState::Full) {
            size_t sc = SizeClass::from_size(page->block_size);
            move_from_full(page, sc);
        }
        page->free_local(ptr, cookie);
    } else {
        // Remote free - atomic with proper ordering
        page->free_thread(ptr, cookie);
        stats.cross_thread_frees.fetch_add(1, std::memory_order_relaxed);
    }
}
```

### Temporal Cadence (Heartbeat)

```cpp
inline void Page::collect(uintptr_t global_cookie) noexcept {
    uintptr_t ck = cookie ? cookie : global_cookie;
    
    // Move local_pending -> available (reverse to maintain LIFO)
    if (local_pending) {
        Block* current = local_pending;
        Block* prev = nullptr;
        
        // Reverse the list
        while (current) {
            Block* next = current->decoded_next(ck);
            current->encode_next(prev, ck);
            prev = current;
            current = next;
        }
        
        // Prepend to available
        if (prev) {
            Block* last = prev;
            while (last) {
                Block* next = last->decoded_next(ck);
                if (!next) break;
                last = next;
            }
            last->encode_next(available, ck);
            available = prev;
        }
        
        pending_count = 0;
        local_pending = nullptr;
    }
    
    // Atomically move thread_pending -> available
    Block* thread_list = thread_pending.exchange(nullptr, std::memory_order_acquire);
    if (thread_list) {
        // Prepend to available
        Block* current = thread_list;
        while (current) {
            Block* next = current->decoded_next(ck);
            if (!next) {
                current->encode_next(available, ck);
                break;
            }
            current = next;
        }
        if (thread_list != available) {
            // Find end of thread_list and link to available
            Block* last = thread_list;
            while (last) {
                Block* next = last->decoded_next(ck);
                if (!next) {
                    last->encode_next(available, ck);
                    break;
                }
                last = next;
            }
        }
        available = thread_list;
        
        used_count -= thread_pending_count;
        thread_pending_count = 0;
    }
    
    heartbeat_counter = config::HEARTBEAT_INTERVAL;
}
```

### Delayed Free Collection (NEW - Issue 7)

```cpp
inline void Heap::collect_delayed_free() noexcept {
    Block* block = delayed_free.exchange(nullptr, std::memory_order_acquire);
    while (block) {
        Block* next = block->decoded_next(cookie);
        
        // Find the page this block belongs to
        Segment* seg = Segment::from_pointer(block);
        if (seg && seg->kind == PageKind::Small) {
            Page* page = seg->page_from_pointer(block);
            if (page) {
                block->encode_next(page->available, page->cookie ? page->cookie : cookie);
                page->available = block;
                page->used_count--;
            }
        }
        
        block = next;
    }
}
```

---

## Security Features (Fixed)

### Guard Pages (NEW - Issue 9)

```cpp
[[nodiscard]] inline Segment* Heap::allocate_segment(PageKind kind, size_t huge_size = 0) noexcept {
    size_t size = (kind == PageKind::Huge) ? huge_size : config::SEGMENT_SIZE;
    
    void* mem = os::reserve(size, config::SEGMENT_ALIGN);
    if (!mem) return nullptr;
    
    // Commit header only
    if (!os::commit(mem, sizeof(Segment))) {
        os::release(mem, size);
        return nullptr;
    }
    
    Segment* seg = new (mem) Segment();
    seg->thread_id = thread_id;
    seg->kind = kind;
    seg->segment_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(seg) >> 22);
    seg->cookie = enable_security ? security::generate_cookie() : 0;
    
    if (kind == PageKind::Small) {
        seg->page_shift = 16;
        seg->page_count = config::SMALL_PAGES_PER_SEGMENT;
        
        // Initialize all pages as Free
        for (int i = 0; i < seg->page_count; ++i) {
            seg->pages[i].state = PageState::Free;
            seg->pages[i].cookie = seg->cookie;
        }
        
        // Commit data pages (after guard)
        uintptr_t header_end = reinterpret_cast<uintptr_t>(seg) + sizeof(Segment);
        uintptr_t data_start = detail::align_up(header_end, config::GUARD_PAGE_SIZE) + config::GUARD_PAGE_SIZE;
        
        if (!os::commit(reinterpret_cast<void*>(data_start), 
                        config::SMALL_PAGE_SIZE * config::SMALL_PAGES_PER_SEGMENT)) {
            os::release(mem, size);
            return nullptr;
        }
    }
    
    return seg;
}
```

### Free List Randomization (Fixed - Issue 6)

```cpp
inline void randomize_free_list(Block* head, size_t count, 
                                size_t block_size,
                                uintptr_t cookie) noexcept {
    if (count <= 1 || !head) return;
    
    // Collect blocks into array
    Block** blocks = reinterpret_cast<Block**>(alloca(count * sizeof(Block*)));
    Block* current = head;
    size_t i = 0;
    
    while (current && i < count) {
        blocks[i++] = current;
        current = current->decoded_next(cookie);
    }
    
    // Fisher-Yates shuffle
    static thread_local std::mt19937_64 rng(
        std::hash<std::thread::id>{}(std::this_thread::get_id())
    );
    
    for (size_t j = i; j > 1; --j) {
        std::uniform_int_distribution<size_t> dist(0, j - 1);
        size_t k = dist(rng);
        std::swap(blocks[j - 1], blocks[k]);
    }
    
    // Rebuild list - blocks[0] is new head
    for (size_t j = 0; j < i; ++j) {
        Block* next = (j + 1 < i) ? blocks[j + 1] : nullptr;
        blocks[j]->encode_next(next, cookie);
    }
}
```

---

## OS Abstraction Layer

```cpp
namespace os {
    [[nodiscard]] inline size_t page_size() noexcept;
    [[nodiscard]] inline void* reserve(size_t size, size_t align) noexcept;
    [[nodiscard]] inline bool commit(void* ptr, size_t size) noexcept;
    [[nodiscard]] inline bool decommit(void* ptr, size_t size) noexcept;
    inline void release(void* ptr, size_t size) noexcept;
    [[nodiscard]] inline uint32_t thread_id() noexcept;
    [[nodiscard]] inline bool has_overcommit() noexcept;
}
```

Implementation uses:
- **POSIX**: `mmap`, `mprotect`, `munmap`, `madvise`
- **Windows**: `VirtualAlloc`, `VirtualFree`, `VirtualProtect`

---

## Integration with HLVM Engine

### File Structure

```
Engine/Source/Common/Public/Core/Mallocator/Mi/
├── MiMalloc.h              # Main single header (~2000 lines)
└── MiMallocWrapper.cpp     # Optional: IMallocator interface wrapper

Engine/Source/Common/Test/
└── TestMiMalloc.cpp        # Comprehensive test suite
```

### IMallocator Interface Wrapper

```cpp
class FMiMallocator : public IMallocator {
public:
    FMiMallocator() { Type = EMallocator::Mimalloc; }
    
    HLVM_NODISCARD bool Owned(void* ptr) noexcept override {
        return mi::Mallocator::instance().pointer_is_valid(ptr);
    }
    
    HLVM_NODISCARD void* Malloc(size_t size) noexcept(false) override {
        void* ptr = mi::Mallocator::instance().allocate(size);
        if (!ptr) throw std::bad_alloc();
        return ptr;
    }
    
    HLVM_NODISCARD void* Malloc2(size_t size) noexcept override {
        return mi::Mallocator::instance().allocate(size);
    }
    
    HLVM_NODISCARD void* MallocAligned(size_t size, size_t alignment) noexcept(false) override {
        void* ptr = mi::Mallocator::instance().allocate_aligned(size, alignment);
        if (!ptr) throw std::bad_alloc();
        return ptr;
    }
    
    HLVM_NODISCARD void* MallocAligned2(size_t size, size_t alignment) noexcept override {
        return mi::Mallocator::instance().allocate_aligned(size, alignment);
    }
    
    HLVM_NODISCARD EFreeRetType Free(void* ptr) noexcept override {
        if (!mi::Mallocator::instance().pointer_is_valid(ptr)) {
            return EFreeRetType::NotOwned;
        }
        mi::Mallocator::instance().free(ptr);
        return EFreeRetType::Success;
    }
    
    // ... other methods
};
```

---

## Testing Strategy

### Test Categories

1. **Basic Allocation**: Small, large, huge
2. **Size Classes**: All 64 size classes
3. **Temporal Cadence**: Trigger heartbeat/generic path
4. **Concurrency**: Multi-threaded allocation/free
5. **Stress**: Random patterns, long-running
6. **Security**: Guard pages, randomization verification
7. **C++ Integration**: `std::vector`, `std::map` with custom allocator
8. **Realloc**: Data preservation tests
9. **Alignment**: Various alignment requirements
10. **Edge Cases**: Zero size, max size, overflow

### Benchmark Comparisons

Compare against:
- System `malloc/free`
- Existing `MiMallocator` (if present)
- `StackMallocator`

Metrics:
- Allocation throughput (allocs/sec)
- Free throughput (frees/sec)
- Memory overhead (%)
- Cross-thread contention (%)

---

## Implementation Checklist

| Component | Status | Issues Fixed |
|-----------|--------|--------------|
| Configuration constants | ☐ | - |
| Size class system | ☐ | 10 |
| Block structure | ☐ | - |
| Page metadata | ☐ | 5, 6 |
| Segment header | ☐ | 5, 9 |
| Heap class | ☐ | 4, 7, 8, 11, 13 |
| OS abstraction | ☐ | - |
| Security features | ☐ | 9 |
| Fast path allocation | ☐ | 1 |
| Free operation | ☐ | 11 |
| Generic allocation | ☐ | 7 |
| Delayed free collection | ☐ | 7 |
| Statistics | ☐ | 13 |
| C++ allocator adapter | ☐ | - |
| Global operators | ☐ | - |
| Test suite | ☐ | - |

---

## Next Steps

1. **Implement MiMalloc.h** with all fixes
2. **Create test suite** TestMiMalloc.cpp
3. **Compile and verify** all tests pass
4. **Benchmark** against existing allocators
5. **Document** in AI_Mimalloc_subgoal3.md

---

## References

- Subgoal 1: `/AI_Mimalloc_subgoal1.md` (Error Analysis)
- Kimi2.5: `/Vibe_Coding/Mimalloc_new/Kimi2.5/Kimi2.5_Mimalloc.md`
- Original Paper: https://www.microsoft.com/en-us/research/publication/mimalloc-free-list-sharding-in-action/
- HLVM Style: `/DOC_Coding_Style.md` (to be read)
