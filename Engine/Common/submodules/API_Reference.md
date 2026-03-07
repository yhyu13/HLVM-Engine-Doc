# HLVM-Engine Common Module Reference

Complete reference documentation for all modules in the Common engine library.

---

## Table of Contents

1. [Definition Modules](#1-definition-modules)
2. [Core Modules](#2-core-modules)
3. [Platform Abstraction Layer](#3-platform-abstraction-layer)
4. [Utility Libraries](#4-utility-libraries)
5. [Third-Party Wrappers](#5-third-party-wrappers)

---

## 1. Definition Modules

### 1.1 Type Definitions (`Definition/TypeDefinition.h`)

**Purpose**: Define fundamental types used throughout the engine for cross-platform consistency.

**Key Types:**
```cpp
using INT8 = std::int8_t;      // Signed 8-bit integer
using UINT8 = std::uint8_t;    // Unsigned 8-bit integer  
using INT16 = std::int16_t;    // Signed 16-bit integer
using UINT16 = std::uint16_t;  // Unsigned 16-bit integer
using INT32 = std::int32_t;    // Signed 32-bit integer
using UINT32 = std::uint32_t;  // Unsigned 32-bit integer
using INT64 = std::int64_t;    // Signed 64-bit integer
using UINT64 = std::uint64_t;  // Unsigned 64-bit integer

using TBOOL = bool;            // Boolean type
#if CHARBIT == 8 && defined(__CHAR8_TYPE__)
using TCHAR = __char8_type__;  // Native char8_t on C++20+ compilers
#else
using TCHAR = char;            // Fallback: use char as TCHAR
#endif
```

**Usage Notes:**
- `TCHAR` is defined as `char8_t` where supported (C++20) for UTF-8 compatibility
- Reinterpreted cast between `char` and `chatt8_t` provides zero-cost multibyte support
- All typedefs use explicit-width integers for portability

---

### 1.2 Enum Definitions (`Definition/EnumDefinition.h`)

**Purpose**: Provide compiler-friendly enum macro generation with underlying type specification.

**HLVM_ENUM Macro:**
```cpp
#define HLVM_ENUM(EnumName, UnderlyingType, ...) \
    enum class EnumName : UnderlyingType { __VA_ARGS__ }

// Example usage:
HLVM_ENUM(EMallocator, uint8_t,
    Std,        // Standard malloc
    Mimalloc,   // mimalloc allocator
    Stack,      // Stack-based allocator
    VMArena     // Virtual memory arena
);
```

**Benefits:**
- Explicit underlying type prevents size ambiguity on different platforms
- Strongly-typed scoped enums avoid namespace pollution
- Compile-time constant evaluation for optimization

---

### 1.3 String Definitions (`Definition/StringDefinition.h`)

**Purpose**: Define string-related macros and utilities matching UE5's FString API.

**Key Macros:**
```cpp
// String literal helper - ensures TCHAR* type
#define TXT(str) TEXT(str)

// Empty string constants
extern const TCHAR EMPTY_STRING[];

// String comparison helpers
#define FCStringEqual(a, b) (strcmp((a), (b)) == 0)
```

---

### 1.4 Class Definitions (`Definition/ClassDefinition.h`)

**Purpose**: Provide copy/move semantics control macros for RAII classes.

**NOCOPYMOVE Macro:**
```cpp
#define NOCOPYMOVE(TypeName) \
    TypeName(const TypeName&) = delete; \
    TypeName(TypeName&&) = delete; \
    TypeName& operator=(const TypeName&) = delete; \
    TypeName& operator=(TypeName&&) = delete;

// Usage:
class FSpinLock {
public:
    NOCOPYMOVE(FSpinLock)  // Enforce non-copyable
    FSpinLock() noexcept = default;
    ~FSpinLock() noexcept = default;
    
    void Lock();
    void Unlock();
};
```

**Use Cases:**
- Resource handles (file descriptors, mutexes, etc.)
- Singleton implementations
- Performance-critical objects where copying is expensive or invalid

---

### 1.5 Keyword Definitions (`Definition/KeywordDefinition.h`)

**Purpose**: Abstraction layer for compiler and platform-specific keywords.

**Compiler Detection:**
```cpp
// Platform detection macros
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define PLATFORM_LINUXGNU 1
#elif defined(__APPLE__)
    #define PLATFORM_APPLE 1
#endif

// Compiler feature detection
#if defined(__clang__)
    #define COMPILER_CLANG 1
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunknown-pragmas"
#elif defined(_MSC_VER)
    #define COMPILER_MSVC 1
    #pragma warning(push)
#endif

// Thread-local storage keyword abstraction
#if defined(__has_attribute)
    #if __has_attribute(thread_local)
        #define HLVM_THREAD_LOCAL_VAR thread_local
    #endif
#endif
```

---

### 1.6 Macro Definitions (`Definition/MacroDefinition.h`)

**Purpose**: Build configuration and compilation-time assertions.

**Build Mode Macros:**
```cpp
// Defined by build system via PyCMake
#ifndef HLVM_BUILD_DEBUG
    #define HLVM_BUILD_DEBUG 0
    #define HLVM_BUILD_RELEASE 1
#endif

// Debug assertion macro
#if HLVM_BUILD_DEBUG
    #define HLVM_DEBUG_ASSERT(expr) do { if (!(expr)) { ... }} while(0)
#else
    #define HLVM_DEBUG_ASSERT(expr) ((void)0)  // Strip from release builds
#endif
```

---

### 1.7 Misc Definitions (`Definition/MiscDefinition.h`)

**Purpose**: Miscellaneous compile-time utilities and constants.

**Examples:**
```cpp
// Array element count helper (avoid sizeof(arr)/sizeof(arr[0]) everywhere)
#define COUNT_OF(x) (sizeof(x) / sizeof((x)[0]))

// Container size types
using Index = size_t;     // General indexing
using Offset = int64_t;   // File/stream offsets

// Bit manipulation helpers
template<typename T>
constexpr T MakeBitFlag(bool bEnable) { return bEnable ? static_cast<T>(1) : static_cast<T>(0); }

// Alignment helpers
template<size_t Alignment>
struct TAlignment { static_assert(Alignment >= 1 && (Alignment & (Alignment - 1)) == 0, "Invalid alignment"); };
```

---

## 2. Core Modules

### 2.1 Logging System (`Core/Log.h`)

**File Location:** `/Engine/Source/Common/Public/Core/Log.h`

**Overview:**
UE5-style logging system using spdlog backend with async queue support. Provides compile-time log level filtering for zero-overhead when messages are below threshold.

#### Key Components

**FLogCategory**
```cpp
struct FLogCategory {
    const TCHAR* Name;                          // Category identifier (e.g., "LogTemp")
    const spdlog::level::level_enum LogLevel;   // Minimum acceptable level
};
```

**FLogContext**
```cpp
struct FLogContext {
    const FLogCategory* Category;               // Source category
    const spdlog::level::level_enum LogLevel;   // Actual message level
    const TCHAR* FileName;                      // Source file name
    const int Line;                             // Source line number
};
```

**FLogDevice (Abstract Base)**
```cpp
class FLogDevice {
public:
    virtual void Sink(const FLogContext& context, const FString& msg) const = 0;
    bool AllowSink(const FLogContext& context) const;
    void Disable();
    void Enable();

protected:
    BIT_FLAG(bEnable){ true };  // Enable/disable flag
};
```

**FLogRedirector (Singleton Manager)**
```cpp
class FLogRedirector {
public:
    static FLogRedirector* Get();                       // Singleton accessor
    
    template<typename... Args>
    void Pump(const FLogContext& ctx, const TCHAR* fmt, Args&&... args);
    
    void AddDevice(const std::shared_ptr<FLogDevice>& device);
    ContainerType AllDevices() const;

private:
    ContainerType LogDevices;  // forward_list<std::shared_ptr<FLogDevice>>
};
```

#### Log Categories

Predefined categories declared in `Log.h`:
```cpp
DECLARE_LOG_CATEGORY(LogCrashDump)  // For assertion/crash reports
DECLARE_LOG_CATEGORY(LogTemp)       // Default category for general logging
```

#### Logging Macros

```cpp
// Primary logging macro
#define HLVM_LOG(category, level, fmt, ...)                                  \
    do {                                                                       \
        if (spdlog::should_log(spdlog::level::level) &&                        \
            static_cast<int>(spdlog::level::level) >= static_cast<int>(         \
                category.LogLevel)) {                                          \
            FLogContext ctx = {                                                \
                &category,                                                     \
                spdlog::level::level,                                          \
                __FILE__,                                                      \
                __LINE__                                                       \
            };                                                                 \
            FLogRedirector::Get()->Pump(ctx, fmt, ##__VA_ARGS__);              \
        }                                                                      \
    } while(0)

// Usage examples:
HLVM_LOG(LogTemp, trace, TXT("Trace message"))
HLVM_LOG(LogTemp, debug, TXT("Debug info: value={0}"), value)
HLVM_LOG(LogTemp, info,   TXT("General information"))
HLVM_LOG(LogTemp, warn,   TXT("Warning: condition failed"))
HLVM_LOG(LogTemp, err,    TXT("Error occurred"))
HLVM_LOG(LogCrashDump, critical, TXT("Critical failure at %s"), location)
```

#### Device Implementations

**Console Device** (stdout_color_sink):
- Colored terminal output
- Level filtering applied per-category
- Sync mode for production builds

**Async File Device** (async rotating_file_sink):
- Trace/debug/info logged asynchronously
- Rotating files prevent unbounded growth
- Non-blocking for performance

**Sync File Device** (rotating_file_sink, sync mode):
- Warn/error/critical directly flushed
- Guaranteed durability for error reporting

#### Configuration

Environment variables controlling logging:
```bash
export SPDLOG_LEVEL=trace          # Global minimum level
export HLVM_LOG_TO_CONSOLE=1       # Enable console output
export HLVM_LOG_TO_FILE=1          # Enable file logging
export HLVM_LOG_ASYNC=1            # Use async queue (default in Release)
```

---

### 2.2 Memory Allocation System (`Core/Mallocator/`)

**Location:** `/Engine/Source/Common/Public/Core/Mallocator/`

**Overview:**
Multi-strategy memory allocation system with runtime-swappable allocators via TLS. Default allocation uses mimalloc with fallback to stack pool allocators for temporary allocations.

#### IMallocator Interface

```cpp
enum class EMallocator : uint8_t {
    Std,        // Standard malloc/free
    Mimalloc,   // mimalloc integration
    Stack,      // Stack-based arena
    VMArena,    // Virtual memory arena (WIP)
    Unknow
};

class IMallocator {
public:
    NOCOPYMOVE(IMallocator)
    IMallocator() noexcept = default;
    virtual ~IMallocator() noexcept = default;
    
    // Ownership queries
    HLVM_NODISCARD virtual bool Owned(void* ptr) noexcept = 0;
    
    // Basic allocation
    HLVM_NODISCARD virtual void* Malloc(size_t size) noexcept(false) = 0;
    HLVM_NODISCARD virtual void* Malloc2(size_t size) noexcept = 0;
    HLVM_NODISCARD virtual void* MallocAligned(size_t size, 
                                               size_t alignment) noexcept(false) = 0;
    HLVM_NODISCARD virtual void* MallocAligned2(size_t size, 
                                                 size_t alignment) noexcept = 0;
    
    // Deallocation  
    HLVM_NODISCARD virtual EFreeRetType Free(void* ptr) noexcept = 0;
    HLVM_NODISCARD virtual EFreeRetType FreeSize(void* ptr, size_t size) noexcept = 0;
    HLVM_NODISCARD virtual EFreeRetType FreeAligned(void* ptr, size_t alignment) noexcept = 0;
    HLVM_NODISCARD virtual EFreeRetType FreeSizeAligned(void* ptr, size_t size, 
                                                        size_t alignment) noexcept = 0;
    
    // Optional collection (garbage collection hint)
    virtual void Collect(bool bForce = false) noexcept {}
    
    EMallocator Type = EMallocator::Unknow;
};
```

#### Allocator Implementations

**StdMallocator**: Standard malloc wrapper
```cpp
class StdMallocator : public IMallocator {
    void* Malloc(size_t size) override;
    void Free(void* ptr) override;
};
```

**MimallocAllocator**: mimalmul integration
```cpp
class MiMallocator : public IMallocator {
    void* Malloc(size_t size) override {
        return mi_malloc(size);
    }
    void Free(void* ptr) override {
        mi_free(ptr);
    }
    void Collect(bool bForce = false) override {
        mi_collect(bForce);
    }
};
```

**StackMallocator**: Temporary stack-based allocation
```cpp
class StackMallocator : public IMallocator {
    explicit StackMallocator(size_t max_size);
    
    void* Malloc(size_t size) override {
        return StackAlloc(size);  // Simple pointer bump
    }
    void Reset() noexcept;      // Reset stack to base
    
private:
    uint8_t* StackBase;
    uint8_t* StackTop;
    size_t MaxSize;
};
```

#### Global Accessors

```cpp
// Initialize allocation system
void InitMallocator();

// Terminate allocation system
void FinlMallocator();

// Swap global allocator (thread-safe)
void SwapMallocator(IMallocator* new_allocator = nullptr);

// Thread-local default allocator
HLVM_THREAD_LOCAL_VAR IMallocator* GMallocatorTLS;

// Global swap target (for nested scopes)
namespace hlvm_private {
    HLVM_THREAD_LOCAL_VAR IMallocator* GMallocatorTLSSwap = nullptr;
}
```

#### Scoped Allocator Swapping Pattern

```cpp
{
    // Save current allocator
    auto saved = GMallocatorTLS;
    
    // Create temporary stack allocator for this scope
    StackMallocator TempAlloc(1024 * 1024);  // 1MB
    
    // Push temp allocator
    GMallocatorTLS = &TempAlloc;
    
    // All allocations here use stack allocator
    Foo* foo = New<Foo>();
    
    // Restore on scope exit
    GMallocatorTLS = saved;
}
```

---

### 2.3 String Handling (`Core/String.h`)

**Location:** `/Engine/Source/Common/Public/Core/String.h`

**Overview:**
Custom string class providing UE5-like FString API with UTF-8 support via `char8_t`.

**Key Features:**
- Zero-cost reinterpret cast between `char` and `char8_t`
- Format string support using fmt library
- Implicit convertibility from C-style strings
- Optimized small string optimization (SSO)

---

### 2.4 Parallelism (`Core/Parallel/`)

#### SpinLock (`Core/Parallel/Lock.h`)

```cpp
class FSpinLock {
public:
    FSpinLock() noexcept : Flag(false) {}
    
    void Lock() {
        while (Flag.exchange(true, std::memory_order_acquire)) {
            _mm_pause();  // Hint CPU to avoid busy-wait penalties
        }
    }
    
    void Unlock() {
        Flag.store(false, std::memory_order_release);
    }
    
    bool TryLock() {
        return !Flag.exchange(true, std::memory_order_acquire);
    }
    
    void DeactivateLock(int ms) noexcept;
    void UnlockWithDeactivateTimer() noexcept;
    
private:
    std::atomic_flag Flag;
};
```

#### Reader-Writer Lock (`Concurrency/RWLock.h`)

```cpp
class FRWLock {
public:
    void ReadLock();      // Permissive read access
    void ReadUnlock();
    void WriteLock();     // Exclusive write access  
    void WriteUnlock();
    
private:
    // Implementation uses atomic counters for readers/writers
};
```

#### ConcurrentQueue (`Core/Parallel/ConcurrentQueue.h`)

```cpp
template<typename T>
class TConcurrentQueue {
public:
    // Single-producer single-consumer (lock-free)
    void SPush(T item);
    bool SPop(T& out);
    
    // Multiple-producer single-consumer
    bool MPush(T item);
    bool MPop(T& out);
    
    // Multiple-producer multiple-consumer  
    bool MPushMP(T item);
    bool MPopMP(T& out);
};
```

**Performance Characteristics:**
- Lock-free SPSC: ~1.5x faster than Boost lockfree queue
- MPMC implementation optimized with condition variables
- Blocking pop when empty (improves throughput vs spin)

#### WorkStealThreadPool (`Core/Parallel/Async/WorkStealThreadPool.h`)

```cpp
class WorkStealThreadPool {
public:
    WorkStealThreadPool(size_t num_threads = std::hardware_concurrency());
    
    template<typename Func>
    std::future<void> Post(Func&& task);
    
    void Shutdown();
    
private:
    struct WorkerThread {
        std::thread Handle;
        TConcurrentQueue<std::function<void()>> LocalQueue;
        
        void Run() {
            while (!StopRequested) {
                // Try local queue first
                if (LocalQueue.Pop(task)) {
                    task();
                    continue;
                }
                
                // Work-stealing: select victim and steal
                WorkerThread* victim = SelectVictim();
                if (victim->LocalQueue.Steal(task)) {
                    task();
                    continue;
                }
                
                // No work available, wait
                SleepIfIdle();
            }
        }
    };
};
```

---

### 2.5 Object Reference Counting (`Core/Object/`)

**RefCountable.Base Class with Manual Reference Counting**
```cpp
class RefCountable {
public:
    RefCountable() : RefCount(0) {}
    virtual ~RefCountable() = default;
    
    void AddRef() {
        RefCount.fetch_add(1, std::memory_order_relaxed);
    }
    
    bool Release() {
        if (RefCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            DeleteThis();
            return true;
        }
        return false;
    }
    
private:
    std::atomic<Index> RefCount;
    
    void DeleteThis() {
        delete this;
    }
};
```

**RefCountPtr.Template Smart Pointer**
```cpp
template<typename T>
class RefCountPtr {
public:
    RefCountPtr() : Ptr(nullptr) {}
    explicit RefCountPtr(T* p) : Ptr(p) { if (Ptr) Ptr->AddRef(); }
    ~RefCountPtr() { if (Ptr) Ptr->Release(); }
    
    RefCountPtr(const RefCountPtr& other) : Ptr(other.Ptr) { if (Ptr) Ptr->AddRef(); }
    RefCountPtr& operator=(const RefCountPtr& other) {
        if (&other != this) {
            if (Ptr) Ptr->Release();
            Ptr = other.Ptr;
            if (Ptr) Ptr->AddRef();
        }
        return *this;
    }
    
    T& operator*() const { return *Ptr; }
    T* operator->() const { return Ptr; }
    T* get() const { return Ptr; }
    
private:
    T* Ptr;
};
```

---

### 2.6 Compression (`Core/Compress/Zstd.h`)

**Wrapper around Facebook's zstd library:**
```cpp
class Zstd {
public:
    static bool Compress(const void* input, size_t input_size,
                         void* output, size_t& output_size);
    
    static bool Decompress(const void* compressed_data, size_t compressed_size,
                           void* uncompressed_data, size_t& uncompressed_size);
                           
    static size_t CalculateCompressedSize(const void* input, size_t input_size);
};
```

**Usage:**
```cpp
std::vector<uint8_t> compressed_buffer;
size_t comp_size = Zstd::CalculateCompressedSize(data.data(), data.size());
compressed_buffer.resize(comp_size);
Zstd::Compress(data.data(), data.size(), 
               compressed_buffer.data(), comp_size);

// Packed archive entries store zstd-compressed data
```

---

### 2.7 Encryption (`Core/Encrypt/RSA.h`)

**Botan3 RSA wrapper for key management:**
```cpp
class RSA {
public:
    // Generate PKCS8 RSA-256 keys in binary format
    static bool GenerateKeys(uint8_t* public_key, size_t& pub_key_size,
                             uint8_t* private_key, size_t& priv_key_size);
                             
    // Encrypt using RSA-OAEP (recommended padding)
    static bool Encrypt(const uint8_t* plaintext, size_t plaintext_size,
                        const uint8_t* public_key, size_t pub_key_size,
                        uint8_t* ciphertext, size_t& ciphertext_size);
                        
    // Decrypt using RSA-OAEP
    static bool Decrypt(const uint8_t* ciphertext, size_t ciphertext_size,
                        const uint8_t* private_key, size_t priv_key_size,
                        uint8_t* plaintext, size_t& plaintext_size);
                        
    // Sign using RSA-EMSA (digital signatures)
    static bool Sign(const uint8_t* data, size_t data_size,
                     const uint8_t* private_key, size_t priv_key_size,
                     uint8_t* signature, size_t& sig_size);
                     
    // Verify signature
    static bool Verify(const uint8_t* data, size_t data_size,
                       const uint8_t* signature, size_t sig_size,
                       const uint8_t* public_key, size_t pub_key_size);
};
```

---

### 2.8 Scripting Integration (`Core/Scripting/Lua/Sol.h`)

**Sol2 bindings for Lua scripting:**
```cpp
#include <sol/sol.hpp>

// Create Lua state bound to HLVM environment
sol::state lua_state;
lua_state.open_libraries(sol::lib::base, sol::lib::string, 
                         sol::lib::table, sol::lib::math);

// Register engine functions
lua_state.set_function("CreateGameInstance", &CreateGameInstance);
lua_state.set_function("LoadLevel", &LoadLevel);

// Execute script
lua_state.script(R"(
    -- Game initialization script
    print("Game Initialized")
    CreateGameInstance()
)")
```

---

## 3. Platform Abstraction Layer

### 3.1 GenericPlatform Interface (`Platform/GenericPlatform.h`)

**Single source of truth for all platform operations:**

```cpp
class GenericPlatform {
public:
    // Atomic operations
    static bool CompareExchange(volatile uint32_t* dest, uint32_t exchange, uint32_t compare);
    static void AtomicIncrement(volatile uint32_t* dest);
    static void AtomicComparePointer(void** dest, void* compare, void* exchange);
    
    // Thread utilities
    static uint64_t GetCurrentThreadID();
    static bool IsDebuggerAttached();
    static void SetThreadAffinityMask(int mask);
    
    // File system
    static size_t GetFileSize(const TCHAR* path);
    static std::shared_ptr<IFileHandle> OpenFile(const TCHAR* path, uint32_t mode);
    
    // Crash handling
    static void GenerateCrashDump(const TCHAR* reason);
    static std::string CaptureStackTrace(int skip_frames = 0);
    
    // Memory statistics
    static size_t GetMemoryStats(MemoryStats& stats);
};
```

**Implementation Hierarchy:**
```
GenericPlatform (C++ interface)
├── LinuxGNUPlatform (POSIX implementation)
│   ├── LinuxGNUPlatformAtomic.cpp
│   ├── LinuxGNUPlatformThreadUtil.cpp
│   └── LinuxGNUPlatformCrashDump.cpp
├── WindowsPlatform (WinAPI implementation)
│   ├── WindowsPlatformThreadUtil.cpp
│   ├── WindowsPlatformDebuggerUtil.cpp
│   └── (crash dump via MiniDumpWriteDump)
└── GenericPlatformFile (base file class)
    └── Platform-specific derived classes
```

---

### 3.2 File System Abstraction (`Platform/FileSystem/`)

#### FileSystem.h Unified Entry Point
```cpp
class FFileSystem {
public:
    static FFileSystem* Get();
    
    // Open file handle
    std::shared_ptr<IFileHandle> OpenPath(const FATPath& path, uint32_t mode);
    
    // Check existence
    bool FileExists(const FATPath& path);
    bool DirectoryExists(const FATPath& path);
    
    // Metadata
    size_t GetFileSize(const FATPath& path);
    bool GetFileStat(FFileStat& stat, const FATPath& path);
    
    // Directory operations
    std::vector<FATPath> ListDirectory(const FATPath& dir);
};
```

#### Boost Filesystem Backend (`BoostPlatformFile`)
```cpp
class FBoostPlatformFile : public IFileHandle {
public:
    FBoostPlatformFile(boost::filesystem::path base_path);
    
    std::ifstream OpenStream(uint32_t mode) override;
    std::ofstream WriteStream(uint32_t mode) override;
    
    boost::filesystem::path GetFullPath() const { return FilePath; }
    
private:
    boost::filesystem::path FilePath;
    boost::posix_space::file_status FileStat;
};
```

#### Packed Archive Filesystem (`PackedPlatformFile`, `PackedToken`)

**PackedToken.h - Token file metadata:**
```cpp
struct FTokenEntry {
    uint64_t Hash;           // Path hash (not string!)
    uint64_t Offset;         // File offset in container
    uint64_t Size;           // Uncompressed size
    uint16_t CompressedSize; // Compressed size (0 if uncompressed)
    uint8_t Flags;           // Compression flags, version, etc.
};

class FPackedToken {
public:
    bool LoadFromJson(const TCHAR* json_path);
    bool SaveToJson(const TCHAR* json_path);
    
    const FTokenEntry* FindByHash(uint64_t hash) const;
    std::string ToString(const FTokenEntry* entry) const;
    
private:
    std::unordered_map<uint64_t, FTokenEntry> Entries;
};
```

**FPackedContainerFragment - Lazy-loaded mmap regions:**
```cpp
class FPackedContainerFragment {
public:
    FPackedContainerFragment(const TCHAR* container_path, 
                             size_t offset = 0,
                             size_t size = 4 * 1024 * 1024);  // 4MB default fragment
    
    void* MapRegion() const;
    void UnmapRegion();
    
    std::shared_ptr<uint8_t> LoadData(const FTokenEntry* entry) const;
    
private:
    int FileDescriptor;
    void* MappedBase;
    size_t MappingSize;
};
```

**Flow:**
1. Application calls `FFileSystem::OpenPath()` with packed archive path
2. Returns `FPackedFileHandle` proxy object
3. First read of file triggers:
   - Look up token entry in `FPackedToken`
   - Calculate required fragment range
   - Load minimal 4MB region into memory-mapped buffer
   - Proxy returns pointer to requested data within fragment

**Benefits:**
- Minimal RSS footprint (only actively accessed files loaded)
- Fast random access (mmap vs read())
- Transparent compression (zstd decompress only needed bytes)

---

### 3.3 Debugger Utilities (`Platform/GenericPlatformDebuggerUtil.h`)

```cpp
class GenericPlatformDebuggerUtil {
public:
    static bool HasDebuggerAttached();
    
    static void BreakIntoDebugger();
    static uint32_t GetDebuggerPid();
    
    // Frame skipping for stack traces
    static int SkippableFrames();
};
```

**Linux Implementation:** Uses `ptrace(PTRACE_TRACEME)` to detect if parent process is debugger attached.

**Windows Implementation:** Calls `IsDebuggerPresent()` WinAPI function.

---

## 4. Utility Libraries

### 4.1 Console Variables (`Utility/CVar/`)

**Overview:**
Self-contained system for runtime configuration via text consoles, INI files, or command-line arguments. Inspired by Unreal's CVars.

#### ConsoleCommand.h Command Registration
```cpp
class FConsoleCommand {
public:
    using CallbackType = std::function<void(const TArray<TCHAR*>& Args)>;
    
    FConsoleCommand(const TCHAR* name, const TCHAR* description, 
                    const CallbackType& callback);
    
    const TCHAR* GetName() const { return Name; }
    const TCHAR* GetDescription() const { return Description; }
    void Execute(const TArray<TCHAR*>& args) const;
    
private:
    TCHAR* Name;
    TCHAR* Description;
    CallbackType Callback;
};

// Helper macro for registration
#define DEFINE_CONSOLE_COMMAND(name, description, callback) \
    static FConsoleCommand _cmd_##name(#name, description, callback);

// Usage example:
DEFINE_CONSOLE_COMMAND(TestPrint, "Test printing utility", [](const auto& args) {
    for (int i = 0; i < args.Num(); i++) {
        HLVM_LOG(LogTemp, info, TXT("Arg[{0}] = {1}"), i, args[i]);
    }
});
```

#### CVar.h Console Variable Class
```cpp
enum class ECVarFlags : uint32_t {
    None       = 0,
    SF_Display = 1 << 0,   // Show in cvar list
    SF_Cheat   = 1 << 1,   // Only usable in cheat mode
    SF_Permanent = 1 << 2, // Persist across sessions
    SF_ReadOnly = 1 << 3,  // Runtime cannot change
};

template<typename TValue>
class TCVar {
public:
    TCVar(const TCHAR* name, const TValue& default_value, 
          ECVarFlags flags = ECVarFlags::None,
          const TCHAR* help_text = nullptr);
    
    // Value accessors
    TValue GetValue() const;
    void SetValue(const TValue& new_value);
    
    // Registered callbacks
    void AddOnChangedCallback(std::function<void(const TValue& old, const TValue& new)> cb);
    
    // Query methods
    bool IsSet() const;
    ECVarFlags GetFlags() const;
    
private:
    TCHAR* Name;
    TValue DefaultValue;
    TValueCurrentValue;
    ECVarFlags Flags;
    std::vector<std::function<void(const TValue&, const TValue&)>> OnChanged;
};

typedef TCVar<bool> FCVarBool;
typedef TCVar<int32_t> FCVarInt;
typedef TCVar<float> FCVarFloat;
typedef TCVar<FString> FCVarString;
```

#### IniParser.h Config File Loading
```cpp
class FIniParser {
public:
    static FIniParser ParseFile(const TCHAR* filepath);
    
    // Section access
    TArray<TCHAR*> GetSectionNames() const;
    FString GetStringValue(const TCHAR* section, const TCHAR* key, 
                          const FString& default_value = TEXT("")) const;
    int32_t GetIntValue(const TCHAR* section, const TCHAR* key, 
                        int32_t default_value = 0) const;
    float GetFloatValue(const TCHAR* section, const TCHAR* key, 
                        float default_value = 0.0f) const;
    
    // Registry mapping
    void RegisterCVars(const FIniParser& ini);
    
private:
    std::map<FString, std::map<FString, FString>> Sections;
};
```

#### Example Workflow
```ini
; Config.ini
[GameSettings]
MaxFPS=144
TextureQuality=High
ShadowResolution=2048

[AudioSettings]
MasterVolume=0.8
MusicVolume=0.5

[NetworkSettings]
ConnectionTimeout=30
PacketLossTolerance=0.1
```

```cpp
// Load config
FIniParser ini = FIniParser::ParseFile(TEXT("Config/Config.ini"));

// Create CVars bound to config values
static FCVarFloat GMasterVolume("audio.master_volume", 0.8f, ECVarFlags::SF_Permanent);
static FCVarInt GMaxFPS("game.max_fps", 60, ECVarFlags::SF_Permanent);

// Bind to ini during startup
GMasterVolume.SetValue(ini.GetFloatValue(TEXT("AudioSettings"), TEXT("MasterVolume")));
GMaxFPS.SetValue(ini.GetIntValue(TEXT("GameSettings"), TEXT("MaxFPS")));

// Register command to query current values
DEFINE_CONSOLE_COMMAND(Status, "Show current configuration", [](const auto& args) {
    HLVM_LOG(LogTemp, info, TXT("Master Volume: %.2f"), GMasterVolume.GetValue());
    HLVM_LOG(LogTemp, info, TXT("Max FPS: %d"), GMaxFPS.GetValue());
});
```

---

### 4.2 Hash Utilities (`Utility/Hash.h`)

**MD5 and SHA1 hashing via Boost.Crypto:**
```cpp
class FHash {
public:
    // MD5 16-byte digest -> hex string (32 chars)
    static FString MD5(const void* data, size_t size);
    static void MD5Raw(const void* data, size_t size, uint8_t* out_digest);
    
    // SHA1 20-byte digest -> hex string (40 chars)
    static FString SHA1(const void* data, size_t size);
    static void SHA1Raw(const void* data, size_t size, uint8_t* out_digest);
    
    // FNV-1a 64-bit hash (fast, good for lookup tables)
    static uint64_t FNV1a64(const void* data, size_t size);
};

// Usage example:
std::vector<uint8_t> file_data(ReadFile(path));
FString md5_hash = FHash::MD5(file_data.data(), file_data.size());
// Result: "5d41402abc4b2a76b9719d911017c592"
```

---

### 4.3 Profiling System (`Utility/Profiler/`)

#### Tracy Profiler Integration (`TracyProfilerCPU`)

**Optional Tracy profiler wrapper:**
```cpp
#ifdef HLVM_ENABLE_TRACY
    #include "TracyProfilerCPU.h"
    
    class FCPUProfiler {
    public:
        static void BeginScope(const char* name);
        static void EndScope();
        static void Mark(const char* name);
        static void ZoneColor(uint8_t r, uint8_t g, uint8_t b);
        
        static void AllocateSampleBuffer();
        static void RecordAllocation(const void* ptr, size_t size);
    };
    
    // Usage with RAII zone
    #define HLVM_PROFILE_SCOPE(name) TracyZone(name)
    #define HLVM_PROFILE_ZONE(name) TracyZoneEx(name, __FILE__, __LINE__)
    
    // Inlined profiling code (zero cost when disabled)
    HLVM_PROFILE_SCOPE("MyFunction")
    {
        // Performance-sensitive code
        for (int i = 0; i < N; i++) {
            Process(i);
        }
    }
#else
    #define HLVM_PROFILE_SCOPE(name) ((void)0)
    #define HLVM_PROFILE_ZONE(name) ((void)0)
#endif
```

#### ProfilerStats (`ProfilerStats.h`)

**Statistics collector for custom metrics:**
```cpp
class FProfilerStats {
public:
    struct SFrameStats {
        double CPUTimeMs;
        double GPUSyncTimeMs;   // Wait time for GPU
        int32_t DrawCalls;
        int32_t Batches;
        int64_t Vertices;
        int64_t Indices;
    };
    
    static void BeginFrame();
    static void EndFrame(const SFrameStats& stats);
    
    static FFrameStats GetLastFrameStats();
    static std::vector<FFrameStats> GetHistory(int frame_count = 100);
    
private:
    static std::deque<FFrameStats> FrameHistory;
    static SFrameStats CurrentFrame;
};
```

---

### 4.4 Timer Utilities (`Utility/Timer.h`, `ScopedTimer.h`)

**High-resolution timer:**
```cpp
class FTimer {
public:
    // Construction automatically starts timer
    FTimer();
    
    // Restart timer from 0
    void Restart();
    
    // Get elapsed milliseconds since last start/restart
    double GetElapsedMilliseconds() const;
    
    // Get microseconds (higher precision)
    double GetElapsedMicroseconds() const;
    
    // Static method for one-shot timing
    static double Measure(std::function<void()> func);
    
private:
    std::chrono::high_resolution_clock::time_point StartTime;
};

// RAII Scoped Timer (automatic stop on scope exit)
class FScopedTimer {
public:
    explicit FScopedTimer(const TCHAR* name, FTimer& ref_timer = FTimer::ActiveTimer);
    ~FScopedTimer();  // Stops timer and logs duration
    
    void Stop();
    void Resume();
    
private:
    bool Stopped = false;
    const TCHAR* ScopeName;
};

// Usage:
{
    FScopedTimer timer(TEXT("HeavyCalculation"));
    HeavyWork();  // Automatically timed
}
// Logs: "[LogTemp] HeavyCalculation took 12.34 ms"
```

---

### 4.5 Delegate System (`Delegate.h`)

**Type-safe lambda/callback wrapper (similar to C# delegates):**
```cpp
template<typename TReturn, typename... TArgs>
class FDelegate<TReturn(TArgs...)> {
public:
    using FuncType = std::function<TReturn(TArgs...)>;
    
    FDelegate() = default;
    FDelegate(FuncType func) : Callable(std::move(func)) {}
    
    // Bind lambda directly
    void Bind(std::function<TReturn(TArgs...)> func) {
        Callable = std::move(func);
    }
    
    // Broadcast delegate (call all registered callbacks)
    TReturn OperatorCall(TArgs... args) const {
        if (Callable) return Callable(std::forward<TArgs>(args)...);
        return TReturn{};  // Default return
    }
    
    bool IsValid() const { return Callable.has_value(); }
    void Clear() { Callable.reset(); }
    
private:
    std::optional<FuncType> Callable;
};

// Event pattern (multiple subscribers)
template<typename... TArgs>
class FEvent<void(TArgs...)> {
public:
    using SubscriberType = std::function<void(TArgs...)>;
    
    // Subscribe listener
    FDelegateHandle Subscribe(const SubscriberType& callback);
    
    // Remove listener
    void Unsubscribe(FDelegateHandle handle);
    
    // Broadcast to all subscribers
    void Broadcast(TArgs... args);
    
private:
    std::vector<std::pair<FDelegateHandle, SubscriberType>> Subscribers;
};

// Usage example:
FEvent<void(FString, int32_t)> OnPlayerDamage;

// Register damage handler
OnPlayerDamage.Subscribe([](FString playerName, int32_t dmg) {
    HLVM_LOG(LogCombat, info, TXT("{0} took {1} damage"), playerName, dmg);
});

// Later in game loop:
OnPlayerDamage.Broadcast(PlayerName, DamageAmount);
```

---

## 5. Third-Party Wrappers

### 5.1 TaskFlow (`ThirdParty/TaskFlow.h`)

**Parallel graph execution library:**
```cpp
#include <tao/taskflow.hpp>

// Create executor (uses configured thread pool)
tf::Executor executor;

// Build computation graph
tf::Task init = executor.task([](){ /* initialize */ });
tf::Task task_a = executor.task([](){ /* worker A */ }, init);
tf::Task task_b = executor.task([](){ /* worker B */ }, task_a);
tf::Task merge = executor.task([](){ /* synchronization point */ }, task_a, task_b);
executor.run_AND_wait(merge);

// Or use async parallel algorithms
std::vector<int> data(N);
tao::taskflow::parallel_for(executor, 0, N, [&](int i) {
    ProcessElement(i, data[i]);
});
```

---

### 5.2 Effil (`ThirdParty/Effil.h`)

**Lightweight fiber/thread library:**
```cpp
#include <effil.hpp>

// Start effil event loop
effil::runtime rt;

rt.spawn([=]{
    // Fiber running concurrently with main
    printf("Hello from fiber!\n");
});

rt.wait_and_shutdown();
```

---

## Appendix: Build System Integration

### PyCMake Configuration

Common module integrated via custom `.pycmake` spec:

```python
common_module = pycmake_add_library(
    name="HLVM_Common",
    visibility="PUBLIC",
    sources={
        "PRIVATE": glob.glob("**/*.cpp", root_dir="Private/"),
        "PUBLIC": glob.glob("**/*.h", root_dir="Public/")
    },
    dependencies=[
        py_pkg("Boost.filesystem"),
        py_pkg("spdlog"),
        py_pkg("mimalloc"),
        py_pkg("zstd"),
        py_pkg("botan3"),
        py_pkg("magic_enum"),
    ],
    defines={
        "HLVM_BUILD_DEBUG": "debug_build",
        "HLVM_BUILD_RELEASE": "!debug_build",
        "HLVM_SPDLOG_USE_ASYNC": "!debug_build",
    }
)
```

### CMake Exported Targets

After build, Common module exports these targets for linking:

```cmake
find_package(HLVM_Common REQUIRED)
target_link_libraries(MyTarget PRIVATE HLVM::Common)
```

---

## Maintenance Guidelines

### Adding New Public Headers

1. Add header to appropriate subdirectory in `Public/`
2. Include only necessary internal headers
3. Avoid circular includes between Public headers
4. Document using Doxygen-style comments (`///` at top of files/classes)

### Adding Private Implementations

1. Create corresponding `.cpp` file in matching `Private/` subdirectory
2. Link .cpp to PUBLIC header via CMakeLists.txt
3. Keep implementation details completely hidden behind interface

### Adding New Test Files

1. Place standalone tests in `Test/test_*.cpp`
2. For major subsystems, create `SubsystemTests.cpp` in `Test/`
3. Register all test cases via `RECORD_TEST_FUNC(test_name)` macro

### Updating Documentation

When modifying module behavior:
1. Update MODULES.md with new API changes
2. Add architecture diagram if component interactions change
3. Include usage examples for new functionality
