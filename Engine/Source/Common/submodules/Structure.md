# HLVM-Engine Common Module File Structure

This document describes the complete file structure of the HLVM-Engine Common module.

```
Engine/Source/Common/
├── Public/                               # Public headers (3) - 91 files
│   ├── Common.h                          # Core umbrella header
│   ├── Global.h                          # Global definitions and types  
│   ├── GlobalDefinition.h                # Global macro and type definitions
│   │
│   ├── Definition/                       # Type and keyword definitions
│   │   ├── ClassDefinition.h             # Class macros and definitions
│   │   ├── EnumDefinition.h              # Enum macros (HLVM_ENUM)
│   │   ├── KeywordDefinition.h           # Compiler/platform keywords
│   │   ├── MacroDefinition.h             # Build configuration macros
│   │   ├── MiscDefinition.h              # Miscellaneous definitions
│   │   ├── StringDefinition.h            # String type and macros
│   │   └── TypeDefinition.h              # Fundamental type definitions
│   │
│   ├── Core/                             # Core engine functionality
│   │   ├── Assert.h                      # Assertion handling and crash reporting
│   │   ├── Compress/                     # Compression utilities (Zstd wrapper)
│   │   │   └── CompressDefinition.h      # Compression type definitions
│   │   ├── Container/                    # Container templates
│   │   │   └── ContainerDefinition.h     # Container macros and base definitions
│   │   ├── CoreMinimal.h                 # Minimal core includes
│   │   ├── Delegate.h                    # Delegate/callback system
│   │   ├── Encrypt/                      # Encryption utilities (RSA/Botan3)
│   │   │   └── EncryptDefinition.h       # Encryption type definitions
│   │   ├── Log.h                         # Logging system (UE5-style logs)
│   │   ├── Mallocator/                   # Memory allocation system
│   │   │   ├── IMallocator.h             # Allocator interface class
│   │   │   ├── Mallocator.h              # Main allocator implementation
│   │   │   ├── MallocatorDefinition.h    # Allocator type definitions
│   │   │   ├── MiMallocator.h            # Mimalloc wrapper allocator
│   │   │   ├── PMR.h                     # Polymorphic memory resource (C++17)
│   │   │   ├── StackMallocator.h         # Stack-based temporary allocator
│   │   │   ├── StdMallocator.h           # Standard malloc wrapper
│   │   │   └── _Deprecated/VMMallocator/ # Deprecated VM allocator (WIP)
│   │   │       ├── ISmallBinnedAllocator.h
│   │   │       ├── SmallBinnedAllocator.h
│   │   │       ├── VMArena.h             # Virtual memory arena
│   │   │       ├── VMHeap.h              # Heap within arena
│   │   │       ├── VMMallocator.h        # VM allocator main class
│   │   │       ├── VMMallocatorDefinition.h
│   │   │       └── VMAArena.cpp
│   │   ├── Name.h                        # FName-like name pooling
│   │   ├── Object/                       # Reference-counted objects
│   │   │   ├── RefCountable.h            # Base reference counted class
│   │   │   └── RefCountPtr.h             # Smart pointer forRefCountable
│   │   ├── Parallel/                     # Parallelism primitives
│   │   │   ├── Async/                    # Asynchronous task execution
│   │   │   │   ├── Async.h               # Async task launcher
│   │   │   │   ├── AsyncConfig.h         # Async configuration
│   │   │   │   ├── FiberPool.hpp         # Fiber pool implementation
│   │   │   │   ├── TaskQueue.h           # Task queue for async ops
│   │   │   │   ├── WorkStealFiberPool.h  # Work-stealing fiber pool (WIP)
│   │   │   │   └── WorkStealThreadPool.h # Work-stealing thread pool
│   │   │   ├── ConcurrentQueue.h         # Lock-free concurrent queues (SPSC/MPSC/MPMC)
│   │   │   ├── FixedSizeQueue.h          # Fixed size circular queue
│   │   │   ├── Lock.h                    # Spin locks and reader-writer lock
│   │   │   └── ParallelDefinition.h      # Parallelism type definitions
│   │   ├── Scripting/Lua/                # Lua scripting integration
│   │   │   └── Sol.h                     # Sol2 bindings for Lua
│   │   └── String.h                      # Custom string class (FString)
│   │
│   ├── Platform/                         # Platform abstraction layer
│   │   ├── Memory/                       # Platform-specific memory operations
│   │   │   └── MemoryStats.h             # Memory statistics tracking
│   │   ├── PlatformDefinition.h          # Platform type definitions
│   │   ├── Render/                       # Render device platform data
│   │   │   └── RenderStatsData.h         # Render statistics container
│   │   ├── FileSystem/                   # Abstracted file system operations
│   │   │   ├── Boost/                    # Boost filesystem implementation
│   │   │   │   ├── BoostFileStat.h       # File stat using Boost
│   │   │   │   ├── BoostMapFileHandle.h  # Memory-mapped file handle
│   │   │   │   ├── BoostPlatformFile.h   # Platform file using Boost
│   │   │   │   └── BoostStreamFileHandle.h # std::stream file handle
│   │   │   ├── FileHandle.h              # Generic file handle interface
│   │   │   ├── Packed/                   # Packed archive file system
│   │   │   │   ├── PackedContainerFragment.h  # Container fragment proxy
│   │   │   │   ├── PackedDefinition.h     # Packed fs type definitions
│   │   │   │   ├── PackedEntryHandle.h    # Individual entry handle
│   │   │   │   ├── PackedFileHandle.h     # Packed archive file handle
│   │   │   │   ├── PackedPlatformFile.h   # Platform packed file implementation
│   │   │   │   └── PackedToken.h          # Token file management
│   │   │   ├── Path.h                    # Portable path operations (FPath)
│   │   │   ├── FileSystem.h              # Unified file system abstraction
│   │   │   └── FileSystemDefinition.h     # Filesystem type definitions
│   │   ├── GenericPlatform.h             # Main platform abstraction class
│   │   ├── GenericPlatformAtomicPointer.h # Atomic pointer operations
│   │   ├── GenericPlatformCrashDump.h    # Crash dump generation
│   │   ├── GenericPlatformDebuggerUtil.h # Debugger detection utilities
│   │   ├── GenericPlatformFile.h         # Platform file base class
│   │   ├── GenericPlatformMemory.h       # Platform memory operations
│   │   ├── GenericPlatformStackTrace.h   # Stack trace capture
│   │   ├── GenericPlatformThreadUtil.h   # Thread utility functions
│   │   ├── LinuxGNU/                     # Linux-specific implementations
│   │   │   └── LinuxGNUPlatformCrashDump.cpp
│   │   │   └── LinuxGNUPlatformDebuggerUtil.cpp
│   │   │   └── LinuxGNUPlatformThreadUtil.cpp
│   │   └── Windows/                      # Windows-specific implementations
│   │       ├── WindowsPlatformDebuggerUtil.cpp
│   │       └── WindowsPlatformThreadUtil.cpp
│   │
│   ├── ThirdParty/                       # Third-party library wrappers
│   │   ├── Effil.h                       # Effil threading library
│   │   └── TaskFlow.h                    # TaskFlow parallel patterns
│   │
│   └── Utility/                          # Utility libraries
│       ├── CVar/                         # Console variables system (CVars)
│       │   ├── ConsoleCommand.h          # Console command registration
│       │   ├── CVar.h                    # Console variable class
│       │   ├── CVarMacros.h              # CVar declaration macros
│       │   ├── CVarTypes.h               # CVar value type definitions
│       │   └── IniParser.h               # INI file parser for CVars
│       ├── Hash.h                        # Hash utilities (MD5, SHA1 via Boost)
│       ├── Profiler/                     # Performance profiling system
│       │   ├── ProfilerCPU.h             # CPU profiler interface
│       │   ├── ProfilerDefinition.h      # Profiler type definitions
│       │   ├── ProfilerSrcLoc.h          # Source location tracking
│       │   ├── ProfilerStats.h           # Profiling statistics collector
│       │   └── Tracy/                    # Tracy profiler integration
│       │       ├── TracyDefinition.h     # Tracy config macros
│       │       └── TracyProfilerCPU.h    # Tracy CPU profiler wrapper
│       ├── ScopedTimer.h                 # RAII scope timer guard
│       └── Timer.h                       # High-resolution timer utilities
│
├── Private/                              # Private implementation files (86%) - 39 files
│   ├── Core/                             # Core private implementations
│   │   ├── Assert.cpp                    # Assertion handling implementation
│   │   ├── Compress/Zstd.cpp             # Zstd compression wrapper
│   │   ├── Mallocator/Mallocator.cpp     # Main allocator implementation
│   │   │   └── _Deprecated/VMMallocator/
│   │   │       ├── VMArena.cpp           # VM arena implementation
│   │   │       └── VMHeap.cpp            # VM heap implementation
│   │   ├── Parallel/Async/
│   │   │   ├── WorkStealFiberPool.cpp    # Work-stealing fiber pool
│   │   │   └── WorkStealThreadPool.cpp   # Work-stealing thread pool impl
│   │   └── ...                           # Other core cpp files
│   │
│   ├── Platform/                         # Platform implementations
│   │   ├── FileSystem/
│   │   │   ├── Boost/                    # Boost FS implementations
│   │   │   │   ├── BoostFileStat.cpp
│   │   │   │   ├── BoostMapFileHandle.cpp
│   │   │   │   ├── BoostPlatformFile.cpp
│   │   │   │   └── BoostStreamFileHandle.cpp
│   │   │   ├── FileHandle.cpp            # Generic file handle impl
│   │   │   ├── Packed/                   # Packed FS implementations
│   │   │   │   ├── PackedContainerFragment.cpp
│   │   │   │   ├── PackedEntryHandle.cpp
│   │   │   │   ├── PackedFileHandle.cpp
│   │   │   │   ├── PackedPlatformFile.cpp
│   │   │   │   └── PackedToken.cpp
│   │   │   ├── Path.cpp                  # FPath implementation
│   │   │   └── ...
│   │   ├── LinuxGNU/
│   │   │   ├── LinuxGNUPlatformCrashDump.cpp
│   │   │   ├── LinuxGNUPlatformDebuggerUtil.cpp
│   │   │   └── LinuxGNUPlatformThreadUtil.cpp
│   │   ├── Windows/
│   │   │   ├── WindowsPlatformDebuggerUtil.cpp
│   │   │   └── WindowsPlatformThreadUtil.cpp
│   │   ├── GenericPlatform.cpp           # Generic platform impl
│   │   ├── GenericPlatformFile.cpp       # Generic file impl
│   │   ├── GenericPlatformCrashDump.cpp  # Crash dump impl
│   │   ├── GenericPlatformStackTrace.cpp # Stack trace impl
│   │   └── ...
│   │
│   ├── Utility/                          # Utility implementations
│   │   ├── CVar/
│   │   │   ├── CVar.cpp                  # CVar implementation
│   │   │   ├── CVarExample.cpp           # CVar example usage
│   │   │   ├── CVarTypes.cpp             # CVar type registry
│   │   │   ├── ConsoleCommand.cpp        # Command registration impl
│   │   │   └── IniParser.cpp             # INI parser implementation
│   │   ├── Hash.cpp                      # Hash function implementations
│   │   ├── Profiler/ProfilerCPU.cpp      # CPU profiler implementation
│   │   └── ...
│   │
│   ├── Global.cpp                        # Global object implementation
│   ├── ThirdParty/Effil.cpp              # Effil wrapper implementation
│   └── ...
│
└── Test/                                 # Unit test files (30%) - 27 files
    ├── Test.h                            # Test framework header
    ├── Test.cpp                          # Main test runner
    ├── TestCase_*.cpp                    # Individual test case modules
    │   ├── TestMiMalloc.cpp              # Mimalloc allocator tests
    │   ├── TestLuajit.cpp                # LuaJIT binding tests
    │   ├── TestParallel.cpp              # Parallelism tests
    │   ├── TestTaskFlow.cpp              # TaskFlow parallelism tests
    │   ├── TestSol2.cpp                  # Sol2 Lua binding tests
    │   ├── TestString.cpp                # FString tests
    │   ├── TestCVar.cpp                  # Console variable tests
    │   ├── TestFileSystem.cpp            # File system tests
    │   ├── TestLogger.cpp                # Logger tests
    │   ├── TestSerialization.cpp         # Serialization tests
    │   ├── TestException.cpp             # Exception handling tests
    │   ├── TestUtility.cpp               # General utility tests
    │   └── ...
    └── TestLuajit_Data/                  # LuaJIT test data
        └── test/clib/
            ├── ctest.c                   # C test code
            └── cpptest.cpp               # C++ test code
    
    └── test_*.cpp                        # Standalone test files
        ├── test_alloc.cpp                # Allocation tests
        ├── test_align.cpp                # Alignment tests
        ├── test_debug.cpp                # Debug tests  
        ├── test_os.cpp                   # OS-level tests
        └── ...
```

## File Count Summary

| Directory | File Type | Count | Percentage |
|-----------|-----------|-------|------------|
| **Public/** | Headers (.h/.hpp) | ~91 | 100% |
| **Private/** | Implementation (.cpp) | ~39 | 100% |
| **Test/** | Test files (*.cpp/*.h) | ~27 | 100% |
| **Total** | All files | ~157 | - |

## Module Breakdown

1. **Core Modules** (~40 files)
   - Memory allocation system (IMallocator hierarchy, mimalloc, stack allocators)
   - Logging system (spdlog integration, UE5-style log categories)
   - String handling (FString with chat8_t, FPath with boost::filesystem)
   - Parallelism (concurrent queues, work-stealing thread pools, locks)
   - File handling (FStream abstraction, packed archives)

2. **Platform Abstraction** (~25 files)
   - Platform-independent interfaces in `GenericPlatform*`
   - Linux GNU-specific implementations
   - Windows-specific implementations
   - File system abstraction (Boost FS for local, custom for packed archives)

3. **Utility Modules** (~20 files)
   - Console Variables (CVars) system with Ini parsing
   - Profiling with Tracy integration
   - Hash utilities (MD5, SHA1)
   - Timing utilities (ScopedTimer, Timer)

4. **Third-Party Wrappers** (~3 files)
   - TaskFlow (parallel patterns library)
   - Effil (threading library)
   - Various CMake find_package dependencies

5. **Tests** (~27 files)
   - Unit tests covering all major modules
   - Integration tests for Lua/JIT bindings
   - Stress tests for memory allocators
   - Performance benchmarks

## Key Design Patterns

- **RAII**: Resource Acquisition Is Initialization pattern extensively used
- **Singleton**: FLogRedirector, GMallocatorTLS
- **Interfaces**: IMallocator abstract base class
- **Polymorphism**: Runtime allocator swapping via TLS
- **Template Metaprogramming**: Compile-time optimizations with C++20 features
- **Platform Abstraction**: GenericPlatform interface with platform-specific backends
- **Custom Allocators**: Multiple allocator strategies with runtime selection
