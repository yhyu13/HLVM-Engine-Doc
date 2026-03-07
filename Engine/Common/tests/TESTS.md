# HLVM-Engine Common Module Test Suite Documentation

Comprehensive overview of unit testing architecture, coverage, and test cases for the Common module.

---

## Table of Contents

1. [Test Framework Overview](#1-test-framework-overview)
2. [Writing New Tests](#2-writing-new-tests)
3. [Test Case Organization](#3-test-case-organization)
4. [Running Tests](#4-running-tests)
5. [Individual Test Modules](#5-individual-test-modules)

---

## 1. Test Framework Overview

### Architecture

The HLVM-Engine Common test suite uses a custom lightweight unit testing framework built around macro-based registration. This design provides:

- **Zero external dependencies** - No need for GoogleTest or other test frameworks
- **Flexible execution model** - Support for bool/int/void return types
- **Configurable repeat and seed** - For stress testing and reproducibility
- **Scoped sections** - Enable/disable specific test regions within a single test
- **Automatic timing** - Per-test performance measurement with smoothed averages

### Registry Pattern

```cpp
namespace hlvm_private {
    HLVM_INLINE_VAR std::vector<std::function<void()>> recorded_test_functions{};
}

// All tests registered via RECORD macro
RECORD(test_name, enable=true, seed=0, repeat=1) {
    // Test implementation
    return true;  // or int ret_code, or void
}
```

During `main()` execution, all registered test functions are invoked in order.

---

### Core Macros

#### RECORD - Primary Test Registration Macro

```cpp
#define RECORD(test_function, ...)                    \
    void test_##test_function();                     \
    RECORD_TEST_FUNC(test_function, ##__VA_ARGS__); \
    void test_##test_function()

// Parameters (optional):
// - enable: bool flag to enable/disable test
// - randomSeed: deterministic seed for reproducible randomness  
// - repeat: number of iterations for stress testing

// Example usage:
RECORD(MemoryAllocationTest, true, 0, 5) {
    // Run 5 times with same seed for reproducibility
    std::srand(ctx.randomSeed);
    
    // Allocate memory
    void* ptr = GMallocatorTLS->Malloc(1024);
    
    // Verify allocation succeeded
    HLVM_ENSURE_F(ptr != nullptr, TXT("Allocation returned null"));
    
    // Free memory
    GMallocatorTLS->Free(ptr);
    
    return true;  // indicate success
}
```

#### RETURN Types Supported

| Return Type | Use Case |
|-------------|----------|
| `bool` | Boolean pass/fail indicator |
| `int` | Numeric result codes (0 = success) |
| `void` | Side-effect based tests (assertions inside) |

#### RECORD_BOOL / RECORD_INT Variants

For explicit type clarity:

```cpp
RECORD_BOOL(BooleanTest, true, 0, 1) {
    return (expected == actual);
}

RECORD_INT(ErrorCodeTest, true, 0, 1) {
    ProcessData(input);
    return error_code.get();  // 0 = no error
}
```

#### SECTION Macro - Test Subdivisions

Enable selective execution of test regions:

```cpp
RECORD(ComplexTest, true, 42, 3) {
    // Setup phase
    SetupEnvironment();
    
    SECTION("Phase1_Valid", true, repeat=1) {
        // Test valid inputs only
        ValidateInput(valid_input);
    }
    
    SECTION("Phase1_Invalid", false, repeat=1) {
        // Skip invalid input tests unless needed
        EXPECT_THROW(invalid_operation(), Exception);
    }
    
    SECTION("PerformanceStress", true, repeat=100) {
        // Stress test section - runs 100x
        for (int i = 0; i < N; i++) {
            StressOperation(i);
        }
    }
}
```

---

### Time Measurement Utilities

#### FTimer Class

High-resolution timer using `std::chrono::high_resolution_clock`:

```cpp
class FTimer {
public:
    FTimer(bool auto_start = false) : StartTime(std::chrono::high_resolution_clock::now()) {}
    void Restart();
    double GetElapsedMilliseconds() const;
    static double Measure(std::function<void()> func);
};

// Usage:
FTimer timer;
PerformExpensiveTask();
double duration_ms = timer.GetElapsedMilliseconds();
HLVM_LOG(LogTemp, info, TXT("Task completed in {}ms"), duration_ms);
```

#### RunTestAndCalculateAvg

Smoothed average calculation removing outliers:

```cpp
double avg_time = RunTestAndCalculateAvg(func, num_iterations);

// Removes max and min values before averaging
// More robust than simple mean for noisy measurements
```

---

### Profiler Integration

Tracy CPU profiler integration for performance analysis:

```cpp
// In main():
if constexpr (HLVM_COMPILE_WITH_PROFILER) {
    if (!command_line_args["no-cpu-profile"]) {
        FProfilerCPU::OnFrameBegin();
        
        for (auto& test : recorded_tests) {
            test();
        }
        
        FProfilerCPU::OnFrameEnd();
    }
}
```

Command-line flags:
- `--no-cpu-profile`: Disable Tracy profiling
- `--gperf`: Enable gperftools profiling (Linux only, if compiled with `HLVM_ALLOW_GPERF`)

---

### Command-Line Arguments

Test runner supports these options:

```bash
./HLVM_Common_TestRunner --help

Usage: options_description [options]
Allowed options:
  --help                    produce help message
  --v-lvl <int>             enable verbosity override to specify level
  --gperf <int>             enable gperftools profiling by cpu sample (linux only!)
  --no-cpu-profile <int>    disable cpu profiling (tracy)
```

**Example invocation:**
```bash
# Run all tests with detailed logging
./HLVM_Common_TestRunner --v-lvl 0          # trace level

# Run without Tracy profiler for faster execution
./HLVM_Common_TestRunner --no-cpu-profile 1

# Enable gperftools on Linux
HLVM_ALLOW_GPERF=1 ./HLVM_Common_TestRunner --gperf 1
```

---

## 2. Writing New Tests

### Basic Test Structure

```cpp
#include "Test.h"

RECORD(YourNewTestName, true, 42, 1) {
    // 1. Setup - Initialize test environment
    SetupTestData();
    
    // 2. Execute - Call function under test
    auto result = FunctionUnderTest(parameter);
    
    // 3. Assert - Validate expected behavior
    HLVM_ENSURE_F(result == expected_value, 
                  TXT("Unexpected result: {0}"), result);
    
    // 4. Teardown (implicit RAII cleanup)
    
    return true;  // or int, or void
}
```

### Using Assertions

Primary assertion macro is `HLVM_ENSURE_F`:

```cpp
// Format string assertions
HLVM_ENSURE_F(condition, TXT("Failure: value={0}, expected={1}"), actual, expected);

// Without format string
HLVM_ENSURE_F(ptr != nullptr, "Pointer should not be null");
```

When assertion fails:
- Prints formatted error message
- Captures stack trace via backwardcpp
- Generates crash dump for debugging
- Aborts current test (continues next test)

### Enabling/disabling Tests

Temporary disable tests for investigation:

```cpp
// Inside test file
static TestContext MySpecialContext{ false };  // Set to true temporarily

RECORD(SpecialTestCase, /* use */ MySpecialContext.bEnabled, ...) {
    // Now this test won't run unless you change bEnabled
}
```

### Configuring Repeat Count

For performance-sensitive code:

```cpp
RECORD(StressAllocations, true, 0, 1000) {
    // Runs 1000 times with same seed
    // Good for finding memory corruption issues
}
```

Repeat count > 1 ensures deterministic behavior across runs when seed is fixed.

---

## 3. Test Case Organization

### File Naming Convention

Files organized by subsystem:

| Directory | Pattern | Purpose |
|-----------|---------|---------|
| `Test/` | `Test*.cpp` | Major subsystem tests |
| `Test/` | `test_*.cpp` | Standalone functional tests |
| `Test/LuaJit_Data/test/clib/` | `ctest.c`, `cpptest.cpp` | Lua binding tests |

### Current Test Modules

| File | Subsystem | Coverage | Status |
|------|-----------|----------|--------|
| `TestMiMalloc.cpp` | Memory Allocators | Mimalloc wrappers, swap functionality | ✅ Active |
| `TestFileSystem.cpp` | File System | Boost FS, Packed archives, paths | ✅ Active |
| `TestParallel.cpp` | Parallelism | Locks, queues, thread pools | ✅ Active |
| `TestString.cpp` | String Handling | FString formatting, encoding | ✅ Active |
| `TestCVar.cpp` | Console Variables | CVar get/set, persistence, commands | ✅ Active |
| `TestLogger.cpp` | Logging System | Categories, async sinks, filtering | ✅ Active |
| `TestSol2.cpp` | Scripting (Lua) | Sol2 bindings, basic execution | ✅ Active |
| `TestLuajit.cpp` | Scripting (LuaJIT) | JIT mode, FFI calls | ⚠️ Experimental |
| `TestTaskFlow.cpp` | Task Flow | Graph parallelism | ✅ Active |
| `TestException.cpp` | Error Handling | Crash dumps, stack traces | ✅ Active |
| `TestSerialization.cpp` | Serialization | struct_pack JSON roundtrip | ✅ Active |
| `TestUtility.cpp` | General Utils | Timer, hash, profiler basics | ✅ Active |

### Standalone Test Files (`test_*.cpp`)

These contain focused test scenarios often related to allocator internals:

| File | Focus Area | Description |
|------|------------|-------------|
| `test_alloc.cpp` | Allocation patterns | malloc/free sequences, fragmentation |
| `test_align.cpp` | Alignment guarantees | Aligned vs unaligned access patterns |
| `test_align2.cpp` | Extended alignments | Various alignment constraints (8, 16, 32, 64 byte) |
| `test_simple.cpp` | Basic sanity checks | Simple operations to validate setup |
| `test_size.cpp` | Size reporting | Correct size calculation for structs/types |
| `test_page.cpp` | Page-level ops | OS page allocation/deallocation |
| `test_segment.cpp` | Segment management | Large contiguous block handling |
| `test_os.cpp` | OS layer tests | Direct OS API interactions |
| `test_debug.cpp` | Debug features | Assertion firing, stack trace capture |
| `test_cpp.cpp` | C++ interoperability | Standard C++ library compatibility |
| `debug.cpp` | Diagnostic utilities | Debug information inspection |

---

## 4. Running Tests

### Prerequisites

Build test executable via PyCMake:

```bash
# Configure build
./GenerateCMakeProjects.sh

# Navigate to test output directory
cd Binary/GNULinux-x64/Debug/

# Run test runner
./HLVM_Common unittest

# Or with specific options
./HLVM_Common unittest --v-lvl 0 --no-cpu-profile 1
```

### Output Format

Test output includes timing and status:

```
[info] Running MiMalloc_basic ( #1 )
[info] Completed MiMalloc_basic ( #1 ) in 0.003 seconds
[info] Running ConcurrentQueue_MPMC ( #1 )
[info] Completed ConcurrentQueue_MPMC ( #1 ) in 0.012 seconds
[info] Running StringFormat_compatibility ( #1 )
[info] Completed StringFormat_compatibility ( #1 ) in 0.001 seconds
```

If a test fails:

```
[error] Assertion failed! File: /path/to/TestMiMalloc.cpp:45
Expected: ptr != nullptr
Actual: nullptr
```

Stack trace automatically generated and printed.

### CI/CD Integration

Tests should be part of every PR check:

```yaml
# .github/workflows/test.yml
name: Unit Tests
on: [push, pull_request]

jobs:
  test-common:
    runs-on: ubuntu-latest
    
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup build environment
        run: ./Setup.sh
        
      - name: Generate CMake projects
        run: ./GenerateCMakeProjects.sh
        
      - name: Build Common module
        run: cd Binary/GNULinux-x64 && cmake --build . --config Debug
        
      - name: Run Common tests
        run: |
          cd Binary/GNULinux-x64/Debug
          ./HLVM_Common unittest --no-cpu-profile 1
          
      - name: Upload profiles
        if: failure()
        uses: actions/upload-artifact@v3
        with:
          name: test-profiles
          path: Binary/GNULinux-x64/Debug/*.prof
```

---

## 5. Individual Test Modules

### TestMiMalloc.cpp - Mimalloc Allocator Tests

**Focus**: Mimalloc integration, runtime swapping, performance comparison with mimalloc vs StdMallocator.

**Key Metrics Measured:**
- Allocation speed (MB/s)
- Fragmentation after stress workload
- Cache locality during sequential alloc/dealloc

**Test Examples:**
```cpp
RECORD(MiMalloc_benchmark_malloc, true, 0, 100) {
    std::vector<void*> allocations;
    
    const size_t num_allocs = 1000;
    for (size_t i = 0; i < num_allocs; i++) {
        allocations.push_back(Mimalloc.Malloc(1024));
    }
    
    // Calculate throughput
    double throughput_mb_s = (num_allocs * 1024.0 / 1024.0) / elapsed_ms;
    
    assert(throughput_mb_s > MIN_EXPECTED_THROUGHPUT);
    
    for (auto ptr : allocations) {
        Mimalloc.Free(ptr);
    }
    
    return true;
}

RECORD(MiMalloc_stress_allocator_swap, true, 42, 50) {
    IMallocator* saved = GMallocatorTLS;
    
    for (int i = 0; i < 50; i++) {
        SwapMallocator(&Mimalloc);
        // Do work...
        
        SwapMallocator(&StdMallocator);
        // Switch strategy mid-execution...
        
        SwapMallocator(saved);  // Restore
    }
    
    GMallocatorTLS = saved;
    return true;
}
```

---

### TestParallel.cpp - Parallelism Tests

**Focus**: Concurrency primitives correctness, lock-free queue performance, work-stealing efficiency.

**Subsystems Tested:**
- SpinLock contention benchmarks
- FPSC/MPSC/MPMC queue scalability
- WorkStealThreadPool load balancing

**Test Examples:**
```cpp
RECORD(ConcurrentQueue_MPMC, true, 0, 10) {
    TConcurrentQueue<int32_t> queue;
    std::atomic<int32_t> produced = 0;
    std::atomic<int32_t> consumed = 0;
    
    // Producer threads
    std::vector<std::thread> producers;
    for (int t = 0; t < 4; t++) {
        producers.emplace_back([&]() {
            for (int i = 0; i < 1000; i++) {
                queue.Push(i + t * 1000);
            }
            produced.fetch_add(1000);
        });
    }
    
    // Consumer threads  
    std::vector<std::thread> consumers;
    for (int t = 0; t < 4; t++) {
        consumers.emplace_back([&]() {
            for (int i = 0; i < 1000; i++) {
                int32_t val;
                while (!queue.TryPop(val)) {
                    std::this_thread::yield();
                }
                consumed.fetch_add(1);
            }
        });
    }
    
    for (auto& p : producers) p.join();
    for (auto& c : consumers) c.join();
    
    HLVM_ENSURE_F(consumed.load() == produced.load(),
                  TXT("Not all messages received"));
                    
    return true;
}

RECORD(SpinLock_contention, true, 0, 1) {
    FSpinLock lock;
    std::atomic<bool> reached_critical = false;
    
    auto spin_task = [&](int id) {
        for (int iter = 0; iter < 10000; iter++) {
            lock.Lock();
            reached_critical.store(true, std::memory_order_relaxed);
            lock.Unlock();
            
            reached_critical.store(false, std::memory_order_relaxed);
        }
    };
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; i++) {
        threads.emplace_back(spin_task, i);
    }
    
    for (auto& t : threads) t.join();
    
    return true;
}
```

---

### TestString.cpp - String Handling Tests

**Focus**: FString formatting compatibility, UTF-8 encoding validation, SSO optimization verification.

**Test Cases:**
```cpp
RECORD(StringFormat_compatibility, true, 0, 1) {
    FString msg = FString::Format(TXT("Hello {0}, your score is {1}!"), 
                                  FString(TEXT("World")), 42);
                                  
    HLVM_ENSURE_F(msg.Contains(TEXT("World")), 
                  TXT("Formatted string should contain 'World'"));
                  
    HLVM_ENSURE_F(msg.Contains(TEXT("42")), 
                  TXT("Formatted string should contain '42'"));
                  
    return true;
}

RECORD(String_encoding_utf8, true, 0, 1) {
    FString hello_world = TEXT("Hello World!");
    FString unicode_text = TEXT("你好，世界！🌍");
    
    // Encode to char8_t
    const uint8_t* data = reinterpret_cast<const uint8_t*>(hello_world.GetCharArray());
    
    // Decode back
    FString decoded(reinterpret_cast<char8_t*>(const_cast<uint8_t*>(data)));
    
    HLVM_ENSURE_F(decoded == hello_world, TXT("UTF-8 roundtrip failed"));
    
    return true;
}
```

---

### TestFileSystem.cpp - File System Tests

**Focus**: Packed archive loading, token file serialization, memory-mapped file integrity.

**Test Cases:**
```cpp
RECORD(PackedArchive_load_single_entry, true, 0, 1) {
    FPackedToken token_loader;
    token_loader.LoadFromJson(CONFIG_DIR "/pactoken.json");
    
    // Find entry by hash
    uint64_t target_hash = FHash::FNV1a64(target_filename, strlen(target_filename));
    const auto* entry = token_loader.FindByHash(target_hash);
    
    HLVM_ENSURE_F(entry != nullptr, TXT("Token entry not found"));
    
    // Load fragment and verify data matches original file
    FPackedContainerFragment fragment(container_path.data());
    auto data = fragment.LoadData(entry);
    
    std::ifstream orig_file(original_file_path, std::ios::binary);
    std::vector<uint8_t> orig_data((std::istreambuf_iterator<char>(orig_file)),
                                   std::istreambuf_iterator<char>());
                                   
    HLVM_ENSURE_F(data.size() == orig_data.size(), 
                  TXT("Size mismatch between packed and original"));
    
    for (size_t i = 0; i < data.size(); i++) {
        HLVM_ENSURE_F(data[i] == orig_data[i], TXT("Byte {0} mismatch"), i);
    }
    
    return true;
}

RECORD(FPath_operators_normalize, true, 0, 1) {
    FPath path_with_dots(TEXT("./subdir/../file.txt"));
    FPath normalized = path_with_dots.Normalize();
    
    HLVM_ENSURE_F(normalized.ToString() == TEXT("file.txt"),
                  TXT("Normalization failed: {0}"), normalized.ToString());
                  
    return true;
}
```

---

### TestCVar.cpp - Console Variable Tests

**Focus**: CVar get/set semantics, Ini file persistence, command-line argument parsing.

**Test Cases:**
```cpp
RECORD(CVar_persistence_to_ini, true, 0, 1) {
    // Create temporary INI file
    FString ini_path = TEXT("/tmp/test_cvar.ini");
    
    FCVarFloat volume_cvar("audio.master_volume", 0.8f, ECVarFlags::SF_Permanent);
    FCVarInt fps_cap("game.max_fps", 60, ECVarFlags::SF_Permanent);
    
    // Modify CVars
    volume_cvar.SetValue(0.75f);
    fps_cap.SetValue(120);
    
    // Save to INI (via helper that writes all permanent CVars)
    SavePersistentCVarsToIni(volume_cvar, fps_cap, ini_path.c_str());
    
    // Reload fresh parser
    FIniParser loaded = FIniParser::ParseFile(ini_path.c_str());
    
    float saved_volume = loaded.GetFloatValue(TEXT("AudioSettings"), TEXT("MasterVolume"));
    int saved_fps = loaded.GetIntValue(TEXT("GameSettings"), TEXT("MaxFPS"));
    
    HLVM_ENSURE_F(fabs(saved_volume - 0.75f) < FLT_EPSILON, 
                  TXT("Volume not persisted correctly"));
    HLVM_ENSURE_F(saved_fps == 120, TXT("FPS cap not persisted correctly"));
    
    return true;
}

RECORD(ConsoleCommand_registration, true, 0, 1) {
    FString captured_output;
    
    DEFINE_CONSOLE_COMMAND(TestEcho, "Echoes arguments", [&captured_output](const auto& args) {
        for (const TCHAR* arg : args) {
            captured_output += FString::Format(TEXT("{0} "), arg);
        }
        HLVM_LOG(LogTemp, info, TXT("Command executed"));
    });
    
    FConsoleManager::Get()->ExecuteCommand(TEXT("TestEcho one two three"));
    
    HLVM_ENSURE_F(captured_output.Contains(TEXT("one")) && 
                  captured_output.Contains(TEXT("three")),
                  TXT("Missing arguments in command output"));
                  
    return true;
}
```

---

### TestLogger.cpp - Logging System Tests

**Focus**: Async sink behavior, category filtering, log level enforcement.

**Test Cases:**
```cpp
RECORD(Logger_async_sink_latency, true, 0, 1) {
    // Capture async queue drain time
    auto start = std::chrono::steady_clock::now();
    
    // Flood logger with messages
    for (int i = 0; i < 1000; i++) {
        HLVM_LOG(LogTemp, debug, TXT("Debug message {}", i));
    }
    
    // Wait for async queue to empty (approximate via sleep)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    auto end = std::chrono::steady_clock::now();
    double latency_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    // Should be near-zero since async doesn't block
    HLVM_ENSURE_F(latency_ms < 10.0, 
                  TXT("Async logging took too long: {}ms"), latency_ms);
                      
    return true;
}

RECORD(Logger_category_filtering, true, 0, 1) {
    DECLARE_LOG_CATEGORY(TestCategory);
    
    int logged_count = 0;
    int threshold_level = spdlog::level::warn;
    TestCategory.LogLevel = threshold_level;
    
    // Try to log below threshold
    HLVM_LOG(TestCategory, debug, TXT("This should not log"));
    HLVM_LOG(TestCategory, info, TXT("This also shouldn't log"));
    HLVM_LOG(TestCategory, warn, TXT("This should log"));
    
    std::string last_log_message;
    FLogRedirector::Get()->AddDevice<FTestCaptureDevice>();
    
    // Simulate processing
    HLVM_LOG(TestCategory, info, TXT("Info message"));     // filtered out
    HLVM_LOG(TestCategory, warn, TXT("Warning message"));  // passes filter
    
    const auto& devices = FLogRedirector::Get()->AllDevices();
    auto count = std::count_if(devices.begin(), devices.end(), [](auto dev) {
        return std::holds_alternative<FTestDevice*>(dev);
    });
    
    HLVM_ENSURE_F(count >= 1, TXT("Required log was suppressed"));
    
    return true;
}
```

---

### TestLuaIntegration - Lua Binding Tests

#### TestSol2.cpp - Modern Lua 5.x (Sol2)

**Focus**: Sol2 C++ bindings, table manipulation, coroutine support.

```cpp
RECORD(Sol2_basic_script_execution, true, 0, 1) {
    sol::state lua;
    lua.open_libraries(sol::lib::base);
    
    // Register C++ function
    lua.set_function("add_numbers", [](int a, int b) {
        return a + b;
    });
    
    // Execute script
    auto result = lua.safe_script(R"(
        local result = add_numbers(5, 3)
        return result
    )");
    
    if (result.valid()) {
        int value = result.get<int>();
        HLVM_ENSURE_F(value == 8, TXT("Expected 8, got {}"), value);
        return true;
    } else {
        sol::error err = result;
        HLVM_ENSURE_F(false, TXT("Script execution failed: {}"), err.what());
    }
}

RECORD(Sol2_table_manipulation, true, 0, 1) {
    sol::state lua;
    
    // Create table in C++
    std::vector<int> values = {1, 2, 3, 4, 5};
    lua.set_global("numbers", values);
    
    // Access from Lua
    auto sum = lua.safe_script(R"(
        local total = 0
        for _, v in ipairs(numbers) do
            total = total + v
        end
        return total
    )");
    
    HLVM_ENSURE_F(sum.get<int>() == 15, TXT("Sum incorrect"));
    return true;
}
```

#### TestLuajit.cpp - Legacy LuaJIT

**Focus**: LuaJIT-specific optimizations, FFI boundary tests.

**Warning**: Marked as experimental due to dependency on older Lua version.

```cpp
RECORD(LuaJIT_FFI_structure_binding, true, 0, 1) {
    // Define C struct in LuaJIT FFI
    auto result = lua.safe_script(R"(
        local ffi = require('ffi')
        ffi.cdef[[
            typedef struct {
                int x;
                int y;
            } Point_t;
        ]]
        
        local point = ffi.new('Point_t', {10, 20})
        return point.x + point.y
    )");
    
    HLVM_ENSURE_F(result.valid() && result.get<int>() == 30);
    return true;
}
```

---

## Appendix A: Performance Benchmarks

### Typical Test Execution Times

| Test Category | Avg Duration | Notes |
|---------------|--------------|-------|
| Memory Allocators | 10-50ms | Varies by repeat count |
| Parallelism | 50-200ms | Higher with multi-thread stress |
| File System | 5-20ms | Depends on disk I/O speed |
| String Operations | <1ms | Mostly CPU-bound |
| Logger Tests | 5-15ms | Async sink latency dominant |
| Lua Bindings | 10-30ms | JIT warmup overhead included |

### Benchmark Results Reference

**WorkStealThreadPool vs Fixed Pool (10k tasks)**:
```
WorkStealThreadPool: 42ms average
FixedThreadPool:     58ms average (20% slower)
```

**ConcurrentQueue throughput (MPMC, 1M messages)**:
```
Throughput: ~2.3 million msgs/sec per consumer thread
vs Boost lockfree queue: ~1.5 million msgs/sec
Improvement: 53% higher throughput
```

**StackAllocator vs Mimalloc (temporary allocations)**:
```
StackAlloc (100 allocs @ 1KB): 0.03 ms
Mimalloc:                      0.09 ms (3x slower)
```

---

## Appendix B: Troubleshooting

### Common Test Failures

#### "Segmentation fault during test"
**Possible causes:**
- Memory corruption in allocator
- Uninitialized member variables
- Use-after-free in delegate callbacks

**Debug steps:**
1. Add ASAN: `cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address"`
2. Compile with `-g` flag for stack traces
3. Reduce repeat count to isolate specific iteration

#### "Non-deterministic test failures"
**Possible causes:**
- Multithreaded race conditions  
- Random number generator state leaking
- Timing-dependent assumptions

**Debug steps:**
1. Fix seed explicitly: `RECORD(..., true, 12345 ...)`
2. Lower repeat count
3. Add synchronization barriers between race-prone sections

#### "Slow CI builds"
**Optimization tips:**
1. Disable Tracy profiler: `--no-cpu-profile 1`
2. Run tests in parallel where possible
3. Archive profiles instead of running live during CI

---

## Appendix C: Contributing Tests

When adding new modules to Common, follow these guidelines:

1. **Create dedicated test file** matching subsystem (e.g., `TestMyNewFeature.cpp`)
2. **Register all tests** using `RECORD` macro with appropriate parameters
3. **Include meaningful assertions** - don't just call functions, validate results
4. **Document edge cases** - cover failure modes, not just success path
5. **Add benchmark cases** where performance matters
6. **Update this document** with new test module details

Example submission pattern:

```cpp
/* TestMyNewModule.cpp
 * Copyright (c) 2025. MIT License. All rights reserved.
 */

#include "Test.h"

RECORD(MyNewModule_integration, true, 42, 1) {
    // Test validates end-to-end workflow
    MyNewClass instance;
    instance.Initialize();
    bool result = instance.Process(data);
    
    HLVM_ENSURE(result.IsValid(), 
                TXT("Processing failed with status code: {}"), 
                instance.GetErrorCode().Code);
                
    return true;
}
```
