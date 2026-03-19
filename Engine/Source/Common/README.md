# HLVM-Engine Common Documentation - Quick Start Guide

Navigation index for Common module documentation.

---

## Documentation Index

| Document | File Path | Description |
|----------|-----------|-------------|
| **Summary** | [../summary.md](../summary.md) | Overview & quick links (start here!) |
| **File Structure** | [./directory/tree.md](./directory/tree.md) | Complete directory tree |
| **Modules API** | [MODULES.md](./MODULES.md) | Full API reference (~4,500 LOC) |
| **Architecture** | [./architecture/architecture.md](./architecture/architecture.md) | System design & diagrams |
| **Tests** | [./tests/TESTS.md](./tests/TESTS.md) | Test framework guide |

---

## Navigation by Role

### New Developer
1. Read [../summary.md](../summary.md)
2. Study [./MODULES.md](./MODULES.md) definitions section
3. Check examples in [./tests/TESTS.md](./tests/TESTS.md)

### Contributor Adding Features
1. Review [./architecture/architecture.md](./architecture/architecture.md) patterns
2. Follow coding style from existing modules
3. Add tests per [./tests/TESTS.md](./tests/TESTS.md) guidelines

### Performance Engineer
1. See benchmark results in [./tests/TESTS.md](./tests/TESTS.md) Appendix B
2. Study WorkStealThreadPool optimization notes
3. Review Module profiling integration docs

### Building Issues
1. Check [../summary.md](../summary.md) Build Commands section
2. Verify prerequisite versions match requirements
3. Consult architecture docs for platform-specific notes

---

## Key Files Reference

```
Document/Engine/Common/
├── summary.md              ← START HERE (overview)
│
├── MODULES.md              ← CORE REFERENCE
│   ├── 1. Definition Modules
│   ├── 2. Core Modules (Logging, Memory, Parallel, etc.)  
│   ├── 3. Platform Abstraction
│   ├── 4. Utility Libraries
│   └── 5. Third-Party Wrappers
│
├── directory/
│   └── tree.md             ← Complete file structure
│
├── architecture/
│   └── architecture.md     ← System diagrams + tradeoffs
│
└── tests/
    └── TESTS.md            ← Test cases + benchmarks
```

---

## Most Referenced APIs

| API | Location in Docs | Example Usage |
|-----|------------------|---------------|
| `HLVM_LOG` macro | MODULES.md §2.1 | `HLVM_LOG(LogTemp, info, TXT("Msg {0}"), val)` |
| `IMallocator` interface | MODULES.md §2.2 | `GMallocatorTLS->Malloc(size)` |
| `FLogRedirector` | MODULES.md §2.1 | `DECLARE_LOG_CATEGORY(MyCat)` |
| `RECORD()` test macro | TESTS.md §2 | `RECORD(TestName, true, seed, repeat)` |
| `WorkStealThreadPool` | MODULES.md §2.4 | `pool.Post([](){work();})` |
| `FCVarFloat` (CVar) | MODULES.md §4.1 | `FCVarFloat vol("volume", 0.8f)` |

---

## External Links

- **HLVM-Engine Main Repo**: https://github.com/yhyu13/HLVM-Engine
- **PyCMake Build Docs**: `bundle/pycmake/` in main repo
- **GitHub Issues**: https://github.com/yhyu13/HLVM-Engine/issues

---

*Generated: March 7, 2026*
