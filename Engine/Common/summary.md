# HLVM-Engine Common Module Documentation Index

Welcome to the Complete Documentation for the HLVM-Engine Common Module.

---

## Overview

The **HLVM-Engine Common** module serves as the foundational infrastructure layer for the HLVM game engine. It implements UE5-inspired system primitives with custom improvements in memory management, file handling, parallelism, and profiling.

### Quick Facts

| Metric | Value |
|--------|-------|
| **Total Files** | ~180 files |
| **Public Headers** | 91 (.h/.hpp) |
| **Private Source** | 39 (.cpp) |
| **Test Files** | 27 (.cpp/.h) |
| **Primary Language** | C++20 |
| **Platform Support** | Linux x64, Windows x64 |
| **Build System** | PyCMake (custom CMake generator) |

### Key Features

1. **UE5-Inspired Logging** - spdlog-based async logging with compile-time level filtering
2. **Custom Memory Allocator** - Mimalloc integration with stack/heap swapping via TLS
3. **Lock-Free Parallelism** - SPSC/MPSC/MPMC queues, work-stealing thread pools
4. **Packed File Archiving** - Token-based asset packaging with mmap compression support
5. **Console Variables** - Full CVar system with INI persistence and command bindings
6. **Profiling Integration** - Tracy CPU profiler + custom statistics collectors
7. **Lua Scripting Support** - Sol2 bindings for modern Lua, LuaJIT support
8. **Cross-Platform Abstraction** - Generic platform layer with Windows/Linux implementations

---

## Documentation Contents

This documentation suite provides comprehensive coverage of the Common module:

### Core Documentation Files

| Document | Location | Description |
|----------|----------|-------------|
| **[tree.md](./directory/tree.md)** | `./directory/tree.md` | Complete file structure with directory hierarchies |
| **[architecture.md](./architecture/architecture.md)** | `./architecture/architecture.md` | System architecture, component diagrams, design tradeoffs |
| **[MODULES.md](./MODULES.md)** | `./MODULES.md` | Detailed API reference for all modules |
| **[TESTS.md](./tests/TESTS.md)** | `./tests/TESTS.md` | Test framework documentation and test case catalog |
| **[summary.md]** (this file) | `./summary.md` | Overview and quick links |

---

## Quick Navigation Guide

### For New Developers

👉 **Start here**: Read [MODULES.md](./MODULES.md) section on "Definitions" for basic types and macros, then proceed to "Core Modules".

👉 **Understand the flow**: Study [architecture.md](./architecture/architecture.md) to see how components interact.

👉 **See it in action**: Check [TESTS.md](./tests/TESTS.md) examples for real-world usage patterns.

### For Contributors Adding New Modules

👉 **Follow patterns**: Review existing modules in MODULES.md before writing new code.

👉 **Add tests first**: See TestMiMalloc.cpp for a complete test case example.

👉 **Update docs**: This index must be updated when new major modules are added.

### For Performance Optimization

👉 **Memory benchmarks**: Look at TestParallel.cpp benchmark results in TESTS.md.

👉 **Profiler usage**: Review utility/Profiler/* headers in MODULES.md.

👉 **Concurrency patterns**: Study WorkStealThreadPool.h implementation notes.

---

## Module Directory Structure

```
Document/Engine/Common/
├── README.md              ← This documentation index
├── tree.md                ← Complete file structure listing
├── MODULES.md             ← Full API reference (1,371 lines)
├── TESTS.md               ← Test framework guide (938 lines)
└── architecture/
    └── architecture.md    ← System architecture & diagrams (446 lines)
```

---

## Codebase Statistics

### File Type Breakdown

| Type | Count | Purpose |
|------|-------|---------|
| `.h` | 91 | Public header files (API definitions) |
| `.hpp` | 1 | Template header (FiberPool) |
| `.tpp` | 14 | Inline template implementations |
| `.cpp` | 66 | Private implementations + tests |
| **.total** | **172** | Active source files |

### Lines of Code (Approximate)

| Component | LOC Quality | Notes |
|-----------|-------------|-------|
| Platform Abstraction | ~8,000 | Cross-platform glue code |
| Core Modules | ~25,000 | Major infrastructure logic |
| Utilities | ~12,000 | Helper libraries |
| Tests | ~15,000 | Unit test suite |
| Documentation | ~4,500 | This doc suite |
| **# total **| **~64,500** | Code + docs |

---

## External Dependencies

### Build Time Dependencies

| Library | Version | License | Usage |
|---------|---------|---------|-------|
| Boost.Filesystem | 1.x | BSL-1.0 | Path operations |
| spdlog | latest | MIT | Logging backend |
| mimalloc | latest | MIT | Default allocator |
| zstd | latest | BSD-3 | Compression |
| Botan3 | latest | Apache 2.0 | RSA encryption |
| Sol2 | latest | MIT | Lua integration |
| Tracy Profiler | latest | zlib | CPU profiling |
| TaskFlow | latest | MIT | Parallel patterns |

### Test Time Dependencies

| Library | Used By | Notes |
|---------|---------|-------|
| gperftools | Optional | Alternative CPU profiler (Linux only) |
| RapidJSON | TestUtil | JSON parsing for config files |

---

## Building the Common Module

### Prerequisites

```bash
# Required systems
Anaconda3          # For package management
git clone ...      # Get source
clang-16           # Compiler (or compatible)
cmake 3.28+        # Build generation

# Optional but recommended
tracy              # CPU profiler client for debugging
asan address-sanitizer  # Memory leak detection
```

### Build Commands

```bash
# Setup environment
./Setup.sh
source ~/anaconda3/etc/profile.d/conda.sh
conda activate hlvm_env

# Generate CMake projects using PyCMake
./GenerateCMakeProjects.sh

# Navigate to build output
cd Binary/GNULinux-x64/Debug

# Build Common library
cmake --build . --target HLVM_Common -j$(nproc)

# Run unit tests
./HLVM_Common unittest --no-cpu-profile 1
```

### Build Flags

Common module supports these build-time configuration options:

| Flag | Default | Effect |
|------|---------|--------|
| `HLVM_BUILD_DEBUG` | 0 | Defines debug symbols, assertions |
| `HLVM_SPDLOG_USE_ASYNC` | 1 (Release) | Enable async logger queue |
| `HLVM_COMPILE_WITH_PROFILER` | 1 | Include Tracy integration |
| `HLVM_ALLOW_GPERF` | 0 | Optionally enable gperftools |

---

## Common Patterns and Idioms

### Log Category Declaration

```cpp
// Define log category in header
DECLARE_LOG_CATEGORY(LogMyFeature)

// Use macro throughout code
HLVM_LOG(MyLog, info, TXT("Message {0} format value={1}"), name, val);
```

### Scoped Allocator Swapping

```cpp
{
    // Save current global allocator
    auto saved = GMallocatorTLS;
    
    // Create temporary stack allocator
    StackMallocator TempAlloc(1024 * 1024);
    
    // Push new allocator
    GMallocatorTLS = &TempAlloc;
    
    // All allocations here use stack
    
    // Restore automatically on scope exit
}
```

### Work Stealing Pattern

```cpp
WorkStealThreadPool pool(num_threads);

// Post task returning std::future for waitability
auto future = pool.Post([]() {
    /* heavy computation */
});

future.wait();  // Block until complete
```

### Console Variable Setup

```cpp
// Create CVar with persistence flag
FCVarFloat GVolume("audio.volume", 0.8f, ECVarFlags::SF_Permanent);

// Load from Ini during initialization
INIParser ini = INIParser::ParseFile(TEXT("Config.ini"));
GVolume.SetValue(ini.GetFloatValue(TEXT("AudioSettings"), TEXT("Volume")));
```

---

## Contribution Guidelines

### Adding New Public APIs

1. Add header to `Engine/Source/Common/Public/YourModule/`
2. Implement in matching `Private/` location  
3. Write test cases in `Test/YourModuleTests.cpp`
4. Document in MODULES.md (update appropriate section)
5. Ensure no type errors: `lsp_diagnostics <filepath>`

### Code Style Expectations

- **Include guards**: `#pragma once` at top of every header
- **Comments**: Doxygen-style (`///`) above functions/classes
- **Namespaces**: Prefer inline namespaces over `using namespace`
- **RAII**: Resource cleanup via destructors (never raw pointers)
- **Type safety**: Avoid `as any` equivalents (`reinterpret_cast`, etc.)

### Testing Requirements

All public interfaces must have corresponding test coverage:
- Positive cases: Verify correct behavior works
- Negative cases: Test failure modes
- Edge cases: Boundary conditions, empty inputs
- Stress tests: High iteration counts for memory/threaded code

---

## Known Issues and Limitations

### WIP Components

| Component | Status | TODO |
|-----------|--------|------|
| VMMallocator | Deprecated WIP | Complete lock-free free list sharding |
| FiberPool | Experimental | Fix work-stealing consistency issues |
| FName Pooling | Planned | Implement atomic ref-counted string pooling |

### Platform-Specific Notes

**Linux GNU:**
- Uses `ptrace(PTRACE_TRACEME)` for debugger detection
- Core dumps captured via signal handlers
- io_uring support under consideration for async I/O

**Windows:**
- Uses WinAPI `IsDebuggerPresent()` for detection  
- MiniDumpWriteDump for crash dumps
- Thread affinity via `SetThreadAffinityMask`

### Third-Party Constraints

- **Boost filesystem**: Heavy dependency slows build times significantly
- **mimalmul**: No built-in fragmentation tracking (may add feature request)
- **Tracy**: Only integrates with Debug/Release builds, not optimized

---

## References

### Internal Resources

- **PyCMake Build System Docs**: See `/bundle/pycmake/` folder
- **HLVM Engine Main README**: Root `/README.md` file
- **Example Usage**: Test subdirectory demonstrates patterns
- **Doxygen Output**: Generated HTML (if build completes): `Binary/*/Doxygen/*`

### External References

- [Unreal Engine Logging Architecture](https://docs.unrealengine.com/5.1/en-US/log-categories-and-system-in-unreal-engine/) - Inspiration for HLVM_LOG macros
- [spdlog Documentation](https://github.com/gabime/spdlog) - Async logging backend
- [Boost Lock-Free Programming](https://www.boost.org/doc/libs/release/libs/fiber/doc/html/fiber.html) - ConcurrentQueue inspirations
- [mimalloc Security Paper](https://pwy.io/posts/mimalloc-cigarette/) - Background on allocator security considerations

---

## Maintenance Checklist

### Before PR Submission

- [ ] All tests pass (`./HLVM_Common unittest`)
- [ ] No compiler warnings (`lsp_diagnostics <changed-file>`)
- [ ] Documentation updated (MODULES.md, summary.md if needed)
- [ ] Build succeeds in Release mode (`cmake --build . --config Release`)

### Quarterly Review Items

- [ ] Update third-party dependency versions
- [ ] Re-run performance benchmarks (work-steal, queues)
- [ ] Verify cross-platform compatibility (Windows/Linux)
- [ ] Audit deprecated components for removal

---

## Contact and Support

For questions about the Common module:
1. Check this documentation thoroughly
2. Search [HLVM-Engine GitHub Issues](https://github.com/yhyu13/HLVM-Engine/issues)
3. Open issue with `[common-module]` tag
4. Join discussion channel (project communication platform)

---

*Last Updated: March 7, 2026*  
*Documentation Version: 0.1.0*  
*Maintained by: HLVM-Engineering Team*
