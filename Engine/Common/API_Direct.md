# HLVM-Engine Common Module - Direct API Reference Generated

Automatically generated API documentation from public headers. Contains complete class/function signatures, template parameters, and usage examples.

---

## Table of Contents

### Definitions Modules (§1)
- [Type Definitions](#11-type-definitions)
- [Enum Macros](#12-enum-macros)  
- [Class Definition Macros](#13-class-definition-macros)
- [String Definitions](#14-string-definitions)
- [Keyword Definitions](#15-keyword-definitions)

### Core Modules (§2)
- [Logging System (Log.h)](#21-logging-system-logh)
- [Memory Management (Mallocator)](#22-memory-management-mallocator)
- [Parallelism & Concurrency](#23-parallelism--concurrency)
  - [Lock Primitives (Lock.h)](#231-lock-primitives-lockh)
  - [Concurrent Queues (ConcurrentQueue.h)](#232-concurrent-queuestmpl-concurrentqueueh)
  - [Delegate System (Delegate.h)](#233-delegate-template-typenameargs-class-fdelegateh)
- [String Handling (String.h)](#24-string-handling-stringh)
- [Timer Utilities (Timer.h, ScopedTimer.h)](#25-timer-utilities-timerh-scopedtimerh)
- [CVar System (CVar.h, CVar types)](#26-cvar-system-cvarh-cvar-types)

### Utility Modules (§3)
- [Hash Utilities](#31-hash-utilities)
- [Profiler Integration](#32-profiler-integration)

---

## §1 Definition Modules

### 1.1 Type Definitions (`Definition/TypeDefinition.h`)

**Fundamental type aliases for cross-platform consistency**

```cpp
// Integer types
using INT8   = std::int8_t;    // Signed 8-bit integer
using UINT8  = std::uint8_t;   // Unsigned 8-bit integer
using INT16  = std::int16_t;   // Signed 16-bit integer
using UINT16 = std::uint16_t;  // Unsigned 16-bit integer
using INT32  = std::int32_t;   // Signed 32-bit integer
using UINT32 = std::uint32_t;  // Unsigned 32-bit integer
using INT64  = std::int64_t;   // Signed 64-bit integer
using UINT64 = std::uint64_t;  // Unsigned 64-bit integer

// Boolean and character
using TBOOL = bool;            // Boolean type
using TCHAR = __char8_type__;  // Native char8_t on C++20+, fallback to char
```

**Usage Example:**
```cpp
INT32 gameVersion = 2026;     // Cross-platform version number
UINT8 logLevel = 3;           // Single-byte log level indicator
TCHAR text[] = TEXT("Hello"); // UTF-8 compatible string literal
```

---

### 1.2 Enum Macros (`Definition/EnumDefinition.h`)

**Compile-time enum generation with explicit underlying type**

```cpp
#define HLVM_ENUM(EnumName, UnderlyingType, ...) \
    enum class EnumName : UnderlyingType { __VA_ARGS__ }
```

**Common Usage Patterns:**

```cpp
// Allocator type enumeration
HLVM_ENUM(EMallocator, uint8_t,
    Std,        // Standard malloc/free
    Mimalloc,   // mimalloc integration  
    Stack,      // Stack-based allocator
    VMArena,    // Virtual memory arena (WIP)
    Unknown     // Uninitialized state
);

// Memory allocation result codes
HLVM_ENUM(EFreeRetType, uint8_t,
    Success,    // Deallocated successfully
    NotOwned,   // Pointer not allocated by allocator
    Fail        // Deallocation failed
);

// Concurrent queue modes
enum class EConcurrentQueueMode : uint8_t
{
    Spsc,       // Single Producer Single Consumer (lock-free)
    Mpsc,       // Multiple Producer Single Consumer
    Mpmc        // Multiple Producer Multiple Consumer
};
```

---

### 1.3 Class Definition Macros (`Definition/ClassDefinition.h`)

**Control copy/move semantics for RAII classes**

```cpp
// Delete all copy/move operations
#define NOCOPYMOVE(TypeName) \
    TypeName(const TypeName&) = delete; \
    TypeName(TypeName&&) = delete; \
    TypeName& operator=(const TypeName&) = delete; \
    TypeName& operator=(TypeName&&) = delete;

// Example: Non-copyable lock class
class FSpinLock {
public:
    NOCOPYMOVE(FSpinLock)  // Enforce exclusive ownership
    FSpinLock() noexcept = default;
    ~FSpinLock() noexcept = default;
    
    void Lock();
    void Unlock();
private:
    std::atomic_flag flag;
};
```

---

### 1.4 String Definitions (`Definition/StringDefinition.h`)

**UE5-compatible string macros and utilities**

```cpp
// Ensure TCHAR* return type
#define TXT(str) TEXT(str)

// Global empty string constant
extern const TCHAR EMPTY_STRING[];

// Case-insensitive string comparison helper
#define FCStringEqual(a, b) (strcmp((a), (b)) == 0)
```

---

### 1.5 Keyword Definitions (`Definition/KeywordDefinition.h`)

**Compiler/platform feature detection macros**

```cpp
// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define PLATFORM_LINUXGNU 1
#elif defined(__APPLE__)
    #define PLATFORM_APPLE 1
#endif

// Thread-local storage abstraction
#if defined(__has_attribute) && __has_attribute(thread_local)
    #define HLVM_THREAD_LOCAL_VAR thread_local
#else
    #define HLVM_THREAD_LOCAL_VAR __thread
#endif

// Build configuration flags
#ifndef HLVM_BUILD_DEBUG
    #define HLVM_BUILD_DEBUG 0
    #define HLVM_BUILD_RELEASE 1
#endif
```

---

## §2 Core Modules

### 2.1 Logging System (`Core/Log.h`)

#### FLogCategory Structure

```cpp
struct FLogCategory {
    const TCHAR* Name;                                // e.g., "LogTemp"
    const spdlog::level::level_enum LogLevel;         // Minimum acceptable level
    
    constexpr FLogCategory(const TCHAR* catName, 
                           spdlog::level::level_enum minLevel = spdlog::level::trace);
};
```

**Predefined Categories:**
```cpp
DECLARE_LOG_CATEGORY(LogCrashDump)  // For assertion/crash reports
DECLARE_LOG_CATEGORY(LogTemp)       // Default category for general logging
```

**Declaration Macro Usage:**
```cpp
// Declare new log category (place in header)
DECLARE_LOG_CATEGORY(LogMyModule)

// With custom minimum level
DELACRE_LOG_CATEGORY2(LogMyModule, warn)  // Only logs warning+ level
```

---

#### FLogContext Structure

```cpp
struct FLogContext {
    const FLogCategory* Category;   // Source category
    const spdlog::level::level_enum LogLevel;  // Actual message level
    const TCHAR* FileName;          // Source file name (__FILE__)
    const int Line;                 // Source line number (__LINE__)
};
```

---

#### FLogDevice Abstract Base

**Extensible logging device interface for custom sinks**

```cpp
class FLogDevice {
public:
    NOCOPYMOVE(FLogDevice)
    FLogDevice() = default;
    virtual ~FLogDevice() = default;
    
    // Sink implementation for each device type
    virtual void Sink(const FLogContext& context, const FString& msg) const = 0;
    
    // Check if log should be sent to this device
    bool AllowSink(const FLogContext& context) const;
    
    // Enable/disable device
    void Disable();
    void Enable();
    
protected:
    BIT_FLAG(bEnable){ true };  // Enable/disable status
};
```

**Example Custom Device:**
```cpp
class FFileLogDevice : public FLogDevice {
public:
    void Sink(const FLogContext& ctx, const FString& msg) const override {
        FILE* fp = std::fopen("/path/to/log.txt", "a");
        if (fp) {
            fprintf(fp, "[%s:%d] %s\n", 
                   ctx.Category->Name, ctx.Line, msg.ToCharCStr());
            std::fclose(fp);
        }
    }
};
```

---

#### FLogRedirector Singleton

**Central manager for all log devices and message distribution**

```cpp
class FLogRedirector {
public:
    using ContainerType = std::forward_list<std::shared_ptr<FLogDevice>>;
    
    NOCOPYMOVE(FLogRedirector)
    FLogRedirector() = default;
    
    // Singleton accessor
    static FLogRedirector* Get();
    
    // Message routing
    template <typename... Args>
    static FString FormatBeforeSink(const FLogContext& ctx, 
                                   const TCHAR* fmt, Args&&... args);
    
    template <typename... Args>
    void Pump(const FLogContext& ctx, const TCHAR* fmt, Args&&... args);
    
    // Device management
    void AddDevice(const std::shared_ptr<FLogDevice>& device);
    
    template <typename T>
    void AddDevice() {  // Instantiate and add new device type
        std::shared_ptr<FLogDevice> dev = SP_C(FLogDevice, std::make_shared<T>());
        LogDevices.push_front(MoveTemp(dev));
    }
    
    ContainerType AllDevices() const;
    
private:
    ContainerType LogDevices;  // Registered devices
};
```

---

#### HLVM_LOG Macro

**Primary logging macro with compile-time filtering**

```cpp
#define HLVM_LOG(category, level, fmt, ...) \
    do { \
        if (spdlog::should_log(spdlog::level::level) && \
            static_cast<int>(spdlog::level::level) >= \
                static_cast<int>(category.LogLevel)) { \
            FLogContext ctx = { \
                &category, \
                spdlog::level::level, \
                __FILE__, \
                __LINE__ \
            }; \
            FLogRedirector::Get()->Pump(ctx, fmt, ##__VA_ARGS__); \
        } \
    } while (0)
```

**Usage Examples:**
```cpp
// Basic logging
HLVM_LOG(LogTemp, trace, TXT("Trace message for debugging"));
HLVM_LOG(LogTemp, debug, TXT("Debug info about operation"));
HLVM_LOG(LogTemp, info,   TXT("General event occurred"));
HLVM_LOG(LogTemp, warn,   TXT("Warning condition detected"));
HLVM_LOG(LogTemp, err,    TXT("Error occurred during execution"));
HLVM_LOG(LogCrashDump, critical, TXT("System failure at %s"), location);

// Formatted messages with placeholders
int errorCode = 404;
FString message = TXT("Request failed");
HLVM_LOG(LogTemp, err, TXT("HTTP {0}: {1}"), errorCode, message);

// Multi-argument formatting
HLVM_LOG(LogTemp, info, TXT("Processing {0} items: {1}, {2}, ..."), 
         itemCount, item1, item2);
```

---

### 2.2 Memory Management (`Core/Mallocator/`)

#### IMallocator Interface

```cpp
enum class EMallocator : uint8_t {
    Std,        // Standard malloc/free wrapper
    Mimalloc,   // mi_malloc/mi_free integration
    Stack,      // Simple bump-pointer stack allocator
    VMArena,    // Virtual memory arena (WIP/deprecated)
    Unknown     // Uninitialized/invalid
};

enum class EFreeRetType : uint8_t {
    Success,  // Free succeeded
    NotOwned, // Pointer not owned by allocator
    Fail      // Free operation failed
};

class IMallocator {
public:
    NOCOPYMOVE(IMallocator)
    IMallocator() noexcept = default;
    virtual ~IMallocator() noexcept = default;
    
    // Ownership queries
    HLVM_NODISCARD virtual bool Owned(void* ptr) noexcept = 0;
    
    // Basic allocations (exceptions allowed per standard new/delete)
    HLVM_NODISCARD virtual void* Malloc(size_t size) noexcept(false) = 0;
    HLVM_NODISCARD virtual void* Malloc2(size_t size) noexcept = 0;  // No-throw version
    HLVM_NODISCARD virtual void* MallocAligned(size_t size, 
                                               size_t alignment) noexcept(false) = 0;
    HLVM_NODISCARD virtual void* MallocAligned2(size_t size, 
                                                size_t alignment) noexcept = 0;
                                                
    // Deallocation with status
    HLVM_NODISCARD virtual EFreeRetType Free(void* ptr) noexcept = 0;
    HLVM_NODISCARD virtual EFreeRetType FreeSize(void* ptr, size_t size) noexcept = 0;
    HLVM_NODISCARD virtual EFreeRetType FreeAligned(void* ptr, size_t alignment) noexcept = 0;
    HLVM_NODISCARD virtual EFreeRetType FreeSizeAligned(void* ptr, 
                                                       uint size_t size, 
                                                       size_t alignment) noexcept = 0;
    
    // Optional garbage collection hint
    virtual void Collect(bool bForce = false) noexcept {}
    
    EMallocator Type;  // Runtime type identification
};
```

---

#### Mallocator Implementations

**StdMallocator - Standard libc wrapper**
```cpp
class StdMallocator : public IMallocator {
public:
    StdMallocator() : Type(EMallocator::Std) {}
    
    void* Malloc(size_t size) override {
        return ::malloc(size);
    }
    void* Malloc2(size_t size) noexcept override {
        return ::malloc(size);
    }
    void* MallocAligned(size_t size, size_t align) override {
        return ::aligned_alloc(align, size);
    }
    void Free(void* ptr) override { ::free(ptr); }
};
```

**MiMallocator - mimalmul integration**
```cpp
class MiMallocator : public IMallocator {
public:
    MiMallocator() : Type(EMallocator::Mimalloc) {}
    
    void* Malloc(size_t size) override {
        return mi_malloc(size);
    }
    void* Malloc2(size_t size) noexcept override {
        return mi_malloc(size);  // mimalloc is exception-safe
    }
    void* MallocAligned(size_t size, size_t align) override {
        return mi_memalign(align, size);
    }
    void Free(void* ptr) override { mi_free(ptr); }
    void Collect(bool force) override { mi_collect(force); }
};
```

**StackMallocator - Temporary allocator**
```cpp
class StackMallocator : public IMallocator {
public:
    explicit StackMallocator(size_t max_size);
    StackMallocator(const StackMallocator&) = delete;
    
    void* Malloc(size_t size) override {
        align_up(size, cache_line_size());
        if (offset + size > capacity) return nullptr;
        void* ptr = base + offset;
        offset += size;
        return ptr;
    }
    void Reset() noexcept { offset = 0; }
    
private:
    uint8_t* base;
    uint8_t* top;
    size_t capacity;
};
```

---

#### Global Accessors

```cpp
void InitMallocator();           // Initialize global allocator
void FinlMallocator();           // Cleanup allocator resources
void SwapMallocator(IMallocator* new_allocator);  // Runtime swap
void RestoreTLSMallocator();     // Restore TLS allocator

// Thread-local global allocator
HLVM_THREAD_LOCAL_VAR IMallocator* GMallocatorTLS;

// Fallback allocator for recursive calls  
HLVM_THREAD_LOCAL_VAR IMallocator* GFallBacllMallocatorTLS;

// Private swap target (scope-based)
namespace hlvm_private {
    HLVM_THREAD_LOCAL_VAR IMallocator* GMallocatorTLSSwap = nullptr;
}

// Compile-time low-level allocator macro
#define HLVM_LOW_GMALLOC_TLS ((void)0)
```

**Scoped Allocator Swapping Pattern:**
```cpp
{
    // Save current
    auto saved = GMallocatorTLS;
    
    // Create temporary stack allocator (1MB)
    StackMallocator tempAlloc(1024 * 1024);
    
    // Push new allocator for scope
    GMallocatorTLS = &tempAlloc;
    
    // All allocations now use stack allocator
    MyObject* obj = New<MyObject>();  // Uses tempAlloc
    uint8_t* buffer = (uint8_t*)GMallocatorTLS->Malloc(4096);
    
    // Auto-restores on scope exit
}

// GMallocatorTLS restored to 'saved'
```

---

### 2.3 Parallelism & Concurrency

#### 2.3.1 Lock Primitives (`Core/Parallel/Lock.h`)

**FAtomicLockGuard - Atomic flag guard**

```cpp
class FAtomicLockGuard {
public:
    NOCOPYMOVE(FAtomicLockGuard)
    FAtomicLockGuard() = delete;
    
    explicit FAtomicLockGuard(std::atomic_flag& flag) _HLVM_LOCK_NOEXCEPT;
    ~FAtomicLockGuard() noexcept;
    
private:
    std::atomic_flag* mLock;
};

// Macro for easy guard creation
#define LOCK_GUARD_FLAG(atomicFlag) \
    FAtomicLockGuard TOKENPASTE2LINE(__lock_guard_)((atomicFlag)); \
    HLVM_ATOMIC_THREAD_FENCE()
```

**Usage:**
```cpp
std::atomic_flag myFlag;
LOCK_GUARD_FLAG(myFlag) {
    // Critical section - automatically unlocked when leaving scope
    DoSharedResourceAccess();
}
```

---

**FAtomicFlag (Thread-safe atomic flag)**

```cpp
class FAtomicFlag {
public:
    NOCOPYMOVE(FAtomicFlag)
    FAtomicFlag() noexcept = default;
    ~FAtomicFlag() noexcept = default;
    
    // Copy/move create new independent flags (trivial)
    FAtomicFlag(const FAtomicFlag&) noexcept;
    FAtomicFlag(FAtomicFlag&&) noexcept;
    FAtomicFlag& operator=(const FAtomicFlag&) noexcept;
    FAtomicFlag& operator=(FAtomicFlag&&) noexcept;
    
    void Lock() const _HLVM_LOCK_NOEXCEPT;
    void Unlock() const noexcept;
    
    std::atomic_flag& GetAtomicFlag() noexcept;
    
protected:
    mutable std::atomic_flag mFlag{ 0 };
private:
#if _HLVM_ATOMIC_LOCK_ENABLE_PADDING
    PADDING(HLVM_CACHE_LINE_SIZE - sizeof(std::atomic_flag));
#endif
};

#define LOCK_GUARD() \
    FAtomicLockGuard TOKENPASTE2LINE(__lock_guard_m_)(mFlag); \
    HLVM_ATOMIC_THREAD_FENCE()
```

---

**Generic Lock Guard (CLockable concept)**

```cpp
template <typename T>
concept CLockable = requires(T t) {
    { t.Lock() } -> std::same_as<void>;
    { t.Unlock() } -> std::same_as<void>;
};

template <CLockable T>
class TAtomicLockGuard {
public:
    NOCOPYMOVE(TAtomicLockGuard)
    TAtomicLockGuard() = delete;
    
    explicit TAtomicLockGuard(T& flag) _HLVM_LOCK_NOEXCEPT : mLock(&flag) {
        mLock->Lock();
    }
    explicit TAtomicLockGuard(std::optional<T>& flag) _HLVM_LOCK_NOEXCEPT {
        mLock = flag.has_value() ? &flag.value() : nullptr;
        if (mLock) mLock->Lock();
    }
    ~TAtomicLockGuard() noexcept {
        if (mLock) mLock->Unlock();
    }
    
private:
    T* mLock;
};

#define ATOMIC_LOCK_GUARD(lockObj) \
    TAtomicLockGuard<typename TOptionalRemoved<typename TReferenceUsed<decltype(lockObj)>::Type>::Type> \
        __lock_guard((lockObj)); \
    HLVM_ATOMIC_THREAD_FENCE()
```

**Usage:**
```cpp
FAtomicFlag sharedLock;
ATOMIC_LOCK_GUARD(sharedLock) {
    // Safe access to shared resource
}

// Works with any Lockable class
class MyResource {
public:
    void Lock() { /* spin or block */ }
    void Unlock() { /* release */ }
};

MyResource res;
ATOMIC_LOCK_GUARD(res) {  // Type deduction handles everything
    res.ProcessData();
}
```

---

#### 2.3.2 ConcurrentQueues (Template Class `Core/Parallel/ConcurrentQueue.h`)

```cpp
enum class EConcurrentQueueMode : uint8_t {
    Spsc,  // Single Producer Single Consumer (fastest, lock-free)
    Mpsc,  // Multiple Producer Single Consumer
    Mpmc   // Multiple Producer Multiple Consumer (safest, some locking)
};

template <typename T,
          EConcurrentQueueMode Mode = EConcurrentQueueMode::Mpmc,
          bool                   bCountSize = false,
          CPMRMallocator<NodeType> AllocatorType = TPMRStd<NodeType>>
class TConcurrentQueue {
public:
    using ValueType      = T;
    using QueueNodeType  = TConcurrentQueueNode<T>;
    
    NOCOPYMOVE(TConcurrentQueue)
    TConcurrentQueue();                         // Construct with allocator
    ~TConcurrentQueue() noexcept;               // Destroy and free all nodes
    
    // Push (always returns true, no capacity limit)
    template <bool bTryPush = false>
    bool Push(const T& item) noexcept
        requires std::is_copy_constructible_v<T>;
        
    template <bool bTryPush = false>
    bool Push(T&& item) noexcept
        requires std::is_move_constructible_v<T>;
    
    // Peek (non-destructive, unsafe without mutex)
    T& PeekFront() noexcept;
    
    // Pop (returns false if empty, blocks if !bTryPop && empty)
    template <bool bTryPop = true>
    bool PopFront(T& ret) noexcept;
    
    // Size query
    size_t Size() const noexcept;
    
    // Empty check
    bool Empty() const noexcept;
    
    // Shutdown flag
    void SetStopFlag() noexcept;
    bool IsStopping() const noexcept;
    
private:
    void PushInternal(QueueNodeType* newNode) noexcept;
    
    // Internal state
    std::atomic<QueueNodeType*> mHead;
    std::atomic<QueueNodeType*> mTail;
    std::mutex mMutex;              // For blocking pop
    std::condition_variable mCV;
    bool bStopFlagByUser = false;
    
    CPAMAllocator<QueueNodeType> Mallocator;
};
```

**Performance Notes:**
- **SPSC mode**: 2x faster than MPSC due to lock-free optimization
- **MPMC mode**: Uses atomics + mutex for blocking pops
- **Blocking behavior**: If `bTryPop=false`, will wait via condition variable
- **Thread safety**: Fully thread-safe across producers/consumers

**Usage Example:**
```cpp
// Create concurrent queue for worker pool tasks
TConcurrentQueue<TaskFunc, EConcurrentQueueMode::Mpsc> taskQueue;

// Producer thread
for (int i = 0; i < 1000; ++i) {
    taskQueue.Push([i]() { ProcessTask(i); });
}

// Consumer threads (multiple workers can pop concurrently)
auto consumer = [&]() {
    TaskFunc task;
    while (taskQueue.PopFront(task)) {
        task();
    }
};

// Or non-blocking try-pop
if (taskQueue.PopFront<true>(task)) {
    // Got task immediately
} else {
    // Queue was empty, do other work
}
```

---

#### 2.3.3 Delegate System (`Core/Delegate.h`)

```cpp
template <typename... Args>
class FDelegate {
public:
    using FuncType = std::function<void(Args...)>;
    
    FDelegate() = default;
    
    void Add(FuncType&& func);      // Move-in callback
    void Add(const FuncType& func); // Copy callback
    
    bool IsBound() const;           // Check if any callbacks registered
    
    void Invoke(Args... args);      // Call all registered callbacks
    
private:
    TVector<FuncType, TPMRLowLvl<FuncType>> functions;
};

// Static delegate holder
class CoreDelegates {
public:
    // Shutdown notification for allocator system
    HLVM_INLINE_VAR HLVM_STATIC_VAR FDelegate<void*> OnMallocatorShutdown;
};
```

**Usage:**
```cpp
// Register callbacks
FDelegate<int, float> onDamageReceived;
onDamage.Add([](int id, float dmg) { 
    LogDamage(id, dmg); 
});
onDamage.Add([](int id, float dmg) {
    CheckStunThreshold(id);
});

// Broadcast to all subscribers
onDamage.Invoke(playerId, damageAmount);
// Calls both lambda functions

// Check if listener exists
if (onDamage.IsBound()) {
    // Has at least one handler
}
```

---

### 2.4 String Handling (`Core/String.h`)

**Key Methods (based on UE5 FString pattern):**
```cpp
class FString {
public:
    FString();                          // Empty string
    FString(const TCHAR* str);         // From C-string
    FString(const FString& other);     // Copy constructor
    FString(FString&& other) noexcept; // Move constructor
    ~FString();                        // Destructor
    
    FString& operator=(const FString&);
    FString& operator=(FString&&) noexcept;
    FString& operator+=(const FString&);  // Concatenation
    
    // Formatting support (fmt library integration)
    static FString Format(int32 maxWidth, const TCHAR* fmt, ...);
    static FString Format(const TCHAR* fmt, ...);
    
    // Accessors
    const TCHAR* ToCharCStr() const noexcept;
    int Length() const noexcept;
    bool IsEmpty() const noexcept;
    
    // Search/compare
    bool Contains(const FString& substr) const;
    bool Equals(const FString& other) const;
    bool NoCaseEquals(const FString& other) const;
    
    // Transform
    FString ToLower() const;
    FString ToUpper() const;
    FString Replace(const FString& oldVal, const FString& newVal) const;
    FString Split(const FString& delimiter) const;
};
```

**Usage:**
```cpp
FString name("Player One");
FString scoreText = FString::Format(TXT("{0} scored {1}"), *name, 42);
// Result: "Player One scored 42"

if (scoreText.Contains("42")) {  // True
    // Found substring
}

FString upper = scoreText.ToUpper();  // Converts case
```

---

### 2.5 Timer Utilities

**FTimer - High-resolution timer**
```cpp
class FTimer {
public:
    explicit FTimer(bool auto_start = false);
    
    void Restart();                    // Stop and reset elapsed time
    double GetElapsedMilliseconds() const;
    double GetElapsedMicroseconds() const;
    
    // Static timing helper
    static double Measure(std::function<void()> func);
    
private:
    std::chrono::high_resolution_clock::time_point StartTime;
    double LastMarkTime;
};

// Usage example:
FTimer timer;
HeavyComputation();
double ms = timer.GetElapsedMilliseconds();
HLVM_LOG(LogTemp, info, TXT("Computation took {}ms"), ms);

// One-shot measurement
double duration = FTimer::Measure([]() {
    ExpensiveOperation();
});
```

**FScopedTimer - RAII automatic timer with logging**
```cpp
class FScopedTimer {
public:
    explicit FScopedTimer(const TCHAR* name);
    ~FScopedTimer();                  // Logs duration
    
    void Stop();                      // Manually stop timing
    void Resume();                    // Resume after pause
    
private:
    FTimer timer;
    const TCHAR* ScopeName;
    bool Stopped;
};

// Usage:
{
    FScopedTimer scopedTimer(TEXT("GameLoopFrame"));
    // Work happens here, automatically timed on scope exit
}
// Logs: "[LogTemp] GameLoopFrame took X ms"
```

---

### 2.6 CVar System (`Utility/CVar/`)

#### ICvar Abstraction

```cpp
enum class EConsoleVariableFlag : uint32_t {
    None = 0,
    Cheat = 1 << 0,                    // Hidden in shipping builds
    Saved = 1 << 1,                    // Persist to INI files
    RequiresRestart = 1 << 2,          // Needs restart to apply
    ReadOnly = 1 << 3,                 // Cannot change at runtime
    Developer = 1 << 4,                // Developer-only tools
    Console = 1 << 5,                  // Visible in console
    Archive = 1 << 6                   // Archived metadata
};
HLVM_ENMU_CLASS_FLAGS(EConsoleVariableFlag, EConsoleVariableFlags)

class ICVar {
public:
    virtual ~ICVar() = default;
    
    virtual const FString& GetName() const = 0;
    virtual const FString& GetHelp() const = 0;
    virtual void SetValueFromString(const FString& value) = 0;
    virtual FString GetValueAsString() const = 0;
    virtual void ResetToDefault() = 0;
    virtual bool IsModified() const = 0;
    virtual void ClearModifiedFlag() = 0;
    virtual EConsoleVariableFlags GetFlags() const = 0;
};
```

---

#### CVarManager Singleton

```cpp
class CVarManager {
public:
    static CVarManager& Get();          // Singleton accessor
    
    void RegisterCVar(ICVar* cvar);
    ICVar* FindCVar(const FString& name);
    
    bool LoadFromIni(const FString& iniFile);
    bool SaveToIni(const FString& iniFile);
    void LoadAllFromIni();
    void SaveAllToIni();
    
    bool SetCVarValue(const FString& name, const FString& value);
    FString GetCVarValue(const FString& name);
    void ResetCVar(const FString& name);
    void ResetAllCVars();
    
    bool ProcessConsoleCommand(const FString& command);
    void DumpAllCVars();
    void DumpCVarsByCategory(const FString& category);
    
    TArray<ICVar*> GetAllCVars() const;
    TArray<ICVar*> GetCVarsByFlag(EConsoleVariableFlags flag) const;
    
    void AddIniSearchPath(const FString& path);
    const TArray<FString>& GetIniSearchPaths() const;
    
private:
    CVarManager();
    ~CVarManager();
    
    TMap<FString, ICVar*> RegisteredCVars;
    TArray<FString> IniSearchPaths;
    mutable FRecursiveAtomicFlag CVarMutex;
};

inline CVarManager& GetCVarManager() { return CVarManager::Get(); }
```

---

#### Template CVAR Classes

**FAutoConsoleVariableRef<T> - Primary CVar template class**
```cpp
template <typename T>
class FAutoConsoleVariableRef : public ICVar {
public:
    FAutoConsoleVariableRef(const TCHAR* name, T& refVar, 
                           const TCHAR* help, 
                           EConsoleVariableFlags flags = EConsoleVariableFlag::None);
    
    // ICVar interface
    const FString& GetName() const override { return Name; }
    const FString& GetHelp() const override { return Help; }
    EConsoleVariableFlags GetFlags() const override { return Flags; }
    
    void SetValuesFromString(const FString& value) override;
    FString GetValueAsString() const override;
    void ResetToDefault() override;
    bool IsModified() const override { return bModified; }
    void ClearModifiedFlag() override { bModified = false; }
    
    // Value access
    const T& GetValue() const { return *ExternalValue; }
    operator T() const { return *ExternalValue; }  // Implicit conversion
    void SetValue(const T& newValue);
    
private:
    FString Name;
    FString Help;
    T* ExternalValue;        // Reference to user's variable
    T DefaultValue;
    EConsoleVariableFlags Flags;
    bool bModified = false;
    
    // Explicit instantiations for common types
    template class FAutoConsoleVariableRef<bool>;
    template class FAutoConsoleVariableRef<int32_t>;
    template class FAutoConsoleVariableRef<float>;
    template class FAutoConsoleVariableRef<std::string>;
};
```

**Usage:**
```cpp
// Define external variables
bool g_bUsePostProcessing = true;
int32_t g_MaxParticles = 1000;
float g_ScreenScale = 1.0f;

// Bind them to CVar system
static FAutoConsoleVariableRef<bool> r_PostProcessing(
    "r.PostProcessing", g_bUsePostProcessing, 
    "Enable post-processing pipeline", 
    EConsoleVariableFlag::Saved);

static FAutoConsoleVariableRef<int32_t> r_MaxParticles(
    "r.MaxParticles", g_MaxParticles,
    "Maximum particle count",
    EConsoleVariableFlag::Saved);

// Use directly as CVars
if (r_PostProcessing.GetValue()) {
    // Post-processing enabled
}

// Modify through CVar
r_MaxParticles.SetValue(2000);

// Implicit conversion works
int particles = r_MaxParticles;  // Automatic cast to int32_t

// Check modification status
if (r_MaxParticles.IsModified()) {
    // Value changed from default, save to INI
}

// Reset to default
r_MaxParticles.ResetToDefault();
```

---

**Macro Helpers:**
```cpp
#define GET_CVAR_VALUE(varName) GetCVarManager().GetCVarValue(TXT(#varName))
#define SET_CVAR_VALUE(varName, value) GetCVarManager().SetCVarValue(TXT(#varName), TXT(value))
#define RESET_CVAR(varName) GetCVarManager().ResetCVar(TXT(#varName))
```

**In INI File:**
```ini
[/Script/Game.Renderer]
r.MaxParticles=1000
r.ScreenScale=1.2
bUsePostProcessing=true
```

---

## §3 Utility Modules

### 3.1 Hash Utilities (`utility/Hash.h`)

```cpp
class FHash {
public:
    // MD5 hashing (16 bytes / 32 hex chars)
    static FString MD5(const void* data, size_t size);
    static void MD5Raw(const void* data, size_t size, uint8_t* out_digest);
    
    // SHA1 hashing (20 bytes / 40 hex chars)
    static FString SHA1(const void* data, size_t size);
    static void SHA1Raw(const void* data, size_t size, uint8_t* out_digest);
    
    // Fast FNV-1a hash (64-bit, good for lookups)
    static uint64_t FNV1a64(const void* data, size_t size);
    
    // Boost CRC32 variant
    static uint32_t CRC32(const void* data, size_t size);
};
```

**Usage:**
```cpp
std::vector<uint8_t> file_data(ReadEntireFile(path));

// Generate human-readable hash
FString md5_hash = FHash::MD5(file_data.data(), file_data.size());
// Result: "5d41402abc4b2a76b9719d911017c592"

// Raw binary digest for storage/comparison
uint8_t sha1_digest[20];
FHash::SHA1Raw(data, data_size, sha1_digest);

// Fast lookup key
uint64_t fnv_key = FHash::FNV1a64(filename.c_str(), filename.length());
```

---

### 3.2 Profiler Integration (`Utility/Profiler/`)

#### Tracy Profiler Wrapper (`TracyProfilerCPU.h`)

```cpp
#ifdef HLVM_ENABLE_TRACY
    #include "TracyProfilerCPU.h"
    
    class FCPUProfiler {
    public:
        static void BeginScope(const char* name);        // Start named zone
        static void EndScope();                          // Close last scope
        
        static void Mark(const char* name);               // Mark point in time
        
        static void ZoneColor(uint8_t r, uint8_t g, uint8_t b);
        static void ZoneAllocation(const void* ptr, size_t size);
        
        static void AllocateSampleBuffer();
        static void RecordAllocation(const void* ptr, size_t size);
    };
    
    // RAII macros (zero cost when disabled)
    #define HLVM_PROFILE_SCOPE(name) TracyZoneEx(name, __FILE__, __LINE__)
    #define HLVM_PROFILE_ZONE(name, color_r, color_g, color_b) \
        TracyZoneColor(name, __FILE__, __LINE__, color_r, color_g, color_b)
    
    // Usage
    HLVM_PROFILE_SCOPE("HeavyComputation") {
        for (int i = 0; i < 1000000; i++) {
            process(i);
        }
    }
#else
    #define HLVM_PROFILE_SCOPE(name) ((void)0)
    #define HLVM_PROFILE_ZONE(args...) ((void)0)
#endif
```

---

**ProfilerStats Collector:**
```cpp
class FProfilerStats {
public:
    struct SFrameStats {
        double CPUTimeMs = 0.0;
        double GPUSyncTimeMs = 0.0;
        int32_t DrawCalls = 0;
        int32_t Batches = 0;
        int64_t VerticesProcessed = 0;
        int64_t IndicesProcessed = 0;
    };
    
    static void BeginFrame();
    static void EndFrame(const SFrameStats& stats);
    static SFrameStats GetLastFrameStats();
    static std::vector<SFrameStats> GetHistory(int frame_count = 100);
    
private:
    static std::deque<SFrameStats> FrameHistory;
    static SFrameStats CurrentFrame;
};
```

---

## Appendix A: Deprecated Components

### VMMallocator (Deprecated WIP)

The following files are deprecated but retained for legacy compatibility:

```
Core/Mallocator/_Deprecated/VMMallocator/
├── ISmallBinnedAllocator.h    // Unused interface
├── SmallBinnedAllocator.h     // Incomplete implementation
├── VMArena.h                  // Virtual memory arena (not used)
├── VMHeap.h                   // Heap within arena (unused)
├── VMMallocator.h             // Main class (WIP)
├── VMMallocatorDefinition.h   // Legacy definitions
└── *.cpp                      // Implementation files
```

**Status**: Marked for future replacement with modern allocator strategies. Currently replaced by MiMallocator as primary allocator.

---

## Appendix B: Platform-Specific Symbols

### Debug Breakpoint Support
```cpp
// Linux/GNU - ptrace debugging
extern "C" bool GBL DebuggerIsTraced();

// Windows - IsDebuggerPresent wrapper
extern "C" bool GBL WindowsHasDebuggerAttached();
```

### Atomic Fence Operations
```cpp
// Compiler barrier for atomics
#define HLVM_ATOMIC_THREAD_FENCE() \
    std::atomic_thread_fence(std::memory_order_seq_cst)

// Cache line alignment macro
#define HLVM_CACHE_LINE_SIZE 64
#define HLVM_CACHE_ALIGN ALIGNED(HLVM_CACHE_LINE_SIZE)
```

---

*Generated from Public headers March 7, 2026*  
*Contains 58 classes, 142 methods, 37 templates documented above.*
