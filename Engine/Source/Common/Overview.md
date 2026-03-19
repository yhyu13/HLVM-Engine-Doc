# HLVM-Engine Common Module Documentation

Documentation for the HLVM-Engine Common module - foundational infrastructure layer providing UE5-inspired primitives for game engine development.

---

## Quick Navigation

| Document | Description | Size |
|----------|-------------|------|
| **[Overview](../summary.md)** | System overview & getting started | 357 LOC |
| **[Architecture](./Architecture.md)** | System design diagrams & component relationships | 266 LOC |
| **[API Reference](./submodules/API_Reference.md)** | Complete module API documentation | **1,370 LOC** |
| **[File Structure](./submodules/Structure.md)** | Directory tree & organization | 131 LOC |
| **[Testing Guide](./tests/TESTS.md)** | Test framework & coverage guide | 937 LOC |

---

## About This Module

The **Common** module serves as the core infrastructure library for HLVM Engine, implementing:

- **UE5-style logging** with compile-time filtering via spdlog
- **Custom memory allocators** (mimalmul integration + stack/swap strategies)
- **Lock-free parallelism** (SPSC/MPSC/MPMC queues, work-stealing thread pools)  
- **Packed file archiving** with token-based lookup and mmap compression
- **Console variables** with INI persistence
- **Cross-platform abstraction** (Linux x64, Windows x64)

**Total Files**: ~180 source files (~65K lines of code)  
**Language**: C++20  
**Platform**: Linux/GNU, Windows  

---

## Module Organization

```
Document/Engine/Common/
├── overview.md              ← Start here!
├── Architecture.md          ← System design & tradeoffs
├── submodules/
│   ├── API_Reference.md     ← Full API documentation
│   └── Structure.md         ← File structure listing
└── tests/
    └── TESTS.md             ← Test framework guide
```

---

## Documentation Contents

### 1. [Overview](../overview.md) ⭐ START HERE

Complete introduction including:
- Quick facts table
- Key features summary  
- Build instructions
- Common patterns and idioms
- Contribution guidelines

### 2. [Architecture](./Architecture.md)

System-wide design documentation featuring:
- Mermaid architecture diagrams
- Memory management hierarchy
- Logging system flowcharts
- Parallelism concurrency model
- Data flow patterns
- Design tradeoff analysis

### 3. [API Reference](./submodules/API_Reference.md) (MASSIVE)

Detailed reference covering all modules:

#### Definition Modules (§1)
- Type Definitions, Enum Macros, String Definitions  
- Class Definition macros, Keyword definitions

#### Core Modules (§2)
- **Log.h** - UE5-style logging with `HLVM_LOG` macro
- **IMallocator** / **MiMallocator** - Memory allocation interface  
- **WorkStealThreadPool** / **ConcurrentQueue** - Parallelism primitives
- **FString** - UTF-8 string handling with SSO
- **RefCountPtr** / **RefCountable** - Smart pointers
- **Zstd**, **RSA** - Compression & encryption wrappers

#### Platform Abstraction (§3)
- **GenericPlatform.h** - Cross-platform operations interface
- **FileSystem.h** - Unified file system (Boost/Packed archives)
- Platform-specific implementations (Windows/Linux)

#### Utility Libraries (§4)
- **CVar.h** - Console variables with IniParser
- **ProfilerCPU.h** - Tracy profiler integration
- **Timer.h** - High-resolution timing

#### Third-Party Wrappers (§5)
- TaskFlow, Effil threading libraries

Includes complete code examples, usage notes, and maintenance guidelines.

### 4. [File Structure](./submodules/Structure.md)

Directory hierarchy showing:
```
Engine/Source/Common/
├── Public/      (91 headers)
│   ├── Core/       (Logging, Memory, Parallel, etc.)
│   ├── Utility/    (CVar, Profiler, Timer)
│   ├── Platform/   (Abstractions + implementations)
│   └── Definition/ (Macros + types)
├── Private/     (Implementation sources)
└── Test/        (Unit tests)
```

### 5. [Testing Guide](./tests/TESTS.md)

Comprehensive test documentation:
- Test framework architecture
- Writing new tests (`RECORD` macro patterns)
- All test case modules catalogued
- Performance benchmarks appendix
- CI/CD integration examples

---

## For Different Users

### New Developer
1. Read [overview](../overview.md)  
2. Study definition modules in [API Reference](./submodules/API_Reference.md) §1
3. Check [TESTS.md](./tests/TESTS.md) examples for implementation patterns

### Contributor 
1. Review [Architecture.md](./Architecture.md) design philosophy
2. Follow coding style from existing modules  
3. Add tests per [TESTS.md](./tests/TESTS.md) requirements

### Performance Engineer
1. See benchmarks in [TESTS.md](./tests/TESTS.md) Appendix B
2. Study [Architecture.md](./Architecture.md) work-stealing section
3. Review PROFILERTNG integration docs

---

## External Links

- **Main Repository**: https://github.com/yhyu13/HLVM-Engine
- **Issue Tracker**: https://github.com/yhyu13/HLVM-Engine/issues
- **Build Docs**: See `bundle/pycmake/` in main repo

---

*Generated: March 7, 2026*  
*Version: 0.1.0*  
