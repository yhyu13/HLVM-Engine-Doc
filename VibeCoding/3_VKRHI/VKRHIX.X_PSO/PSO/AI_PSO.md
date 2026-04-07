GLM5:

	# Task Specification: UE5-Style PSO Cache RHI Framework (NVRHI Vulkan/Linux)
	## 1. Role & Objective
	You are a Senior Graphics Systems Engineer. Your objective is to design and implement a **Pipeline State Object (PSO) Caching Framework** in C++ that mimics the architecture of Unreal Engine 5's RHI layer. This framework must sit on top of the **NVRHI** library, targeting exclusively the **Vulkan** backend on **Linux**.
	## 2. Technical Context
	- **Current Date**: 2026-02-20
	- **Language**: C++20 (Use modern modules/concepts where appropriate, standard STL).
	- **Build System**: CMake (Generate a minimal `CMakeLists.txt`).
	- **Core Dependency**: NVRHI (NVIDIA Render Hardware Interface).
	- **Platform**: Linux (Implement POSIX-compliant file handling).
	- **Graphics API**: Vulkan (via NVRHI abstraction).
	## 3. Architecture Requirements
	### 3.1. RHI Abstraction Layer
	Create a lightweight RHI wrapper that manages the lifecycle of Pipeline State Objects. Do not wrap the entire NVRHI API, only the components necessary for PSO management.
	### 3.2. UE5-Style PSO Caching Strategy
	Implement a caching system similar to UE5's `FPipelineStateCache`. The system must support:
	1.  **Runtime Cache**: An in-memory LRU cache mapping a PSO Descriptor Hash to the NVRHI Pipeline Handle.
	2.  **Disk Persistence**: Serialize the cache to disk to reduce pipeline compilation stutter on subsequent runs.
	3.  **Asynchronous Compilation (Optional but Recommended)**: A job system to compile PSOs on background threads to prevent frame hitching.
	## 4. Detailed Implementation Steps
	### Step 1: PSO Descriptor & Hashing
	- Define a struct `FPSODesc` that wraps NVRHI's pipeline creation parameters (Shader Handles, Input Layouts, Render Target Formats, Depth Stencil State, Blend State, etc.).
	- Implement a robust hashing function (e.g., `cityhash` or `xxhash`) for `FPSODesc`. This hash will serve as the unique key for the PSO.
	- **Requirement**: The hash must be deterministic across application restarts.
	### Step 2: The Cache Manager (`FPSOCache`)
	- Create a class `FPSOCache` that holds:
	    - `std::unordered_map<size_t, nvrhi::GraphicsPipelineHandle>` for graphics pipelines.
	    - `std::unordered_map<size_t, nvrhi::ComputePipelineHandle>` for compute pipelines.
	- **Interface**:
	    ```cpp
	    nvrhi::GraphicsPipelineHandle GetOrCreateGraphicsPipeline(const FPSODesc& Desc);
	    ```
	### Step 3: NVRHI Vulkan Integration
	- Initialize the NVRHI Device with the Vulkan backend.
	- Ensure the pipeline creation logic correctly translates your `FPSODesc` into `nvrhi::GraphicsPipelineDesc` and calls `device->createGraphicsPipeline`.
	### Step 4: Disk Serialization (Linux Specifics)
	- Implement `SaveCache()` and `LoadCache()`.
	- Use standard Linux file paths (e.g., following XDG Base Directory Specification: `$HOME/.local/share/[AppName]/PipelineCache.bin`).
	- Serialize the binary PSO data. If NVRHI provides access to underlying Vulkan cache data (via `vkGetPipelineCacheData`), utilize NVRHI's caching mechanisms if exposed, otherwise serialize the Hash + Creation Parameters.
	## 5. File Structure
	Please generate the following files:
	1.  `CMakeLists.txt`: Dependencies setup (find Vulkan, NVRHI).
	2.  `include/PSOCache.h`: Class definitions.
	3.  `include/PSODesc.h`: Descriptor and Hashing logic.
	4.  `src/PSOCache.cpp`: Implementation.
	5.  `src/Main.cpp`: A demo application proving the concept.
	## 6. Code Quality & Constraints
	- **Memory Management**: Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) and RAII.
	- **Thread Safety**: The `GetOrCreate` method must be thread-safe (use `std::mutex` or `std::shared_mutex`).
	- **Error Handling**: Robust validation of Vulkan/NVRHI creation results.
	- **Comments**: Explain clearly how this mimics UE5's deferred compilation or caching strategy.
	## 7. Output Instruction
	Generate the full content for all files listed in Section 5. Ensure the code compiles on a standard Linux environment (GCC/Clang).

    Kimi2.5

    User: I want give my LLM agent a task to write UE5 like PSO cache RHI framework in cpp using NVRHI Vulkan only backend on linux, you need to reach and genrate a md file for that agent to complete that task
Kimi: I'll help you create a comprehensive markdown specification for implementing a UE5-like PSO (Pipeline State Object) cache RHI framework using NVRHI with Vulkan backend on Linux. Let me first research the current state of NVRHI and UE5's PSO cache implementation to ensure accuracy.  
  Now I have gathered comprehensive information about NVRHI's architecture and UE5's PSO caching system. Let me create a detailed markdown specification for your LLM agent to implement this framework.

```markdown
# UE5-like PSO Cache RHI Framework Specification
## NVRHI Vulkan-Only Backend on Linux

**Target Platform:** Linux (x64/ARM64)  
**Graphics API:** Vulkan 1.3+  
**Abstraction Layer:** NVRHI (NVIDIA Rendering Hardware Interface)  
**Reference Architecture:** Unreal Engine 5 PSO Cache System  
**Language:** C++17/20  
**Build System:** CMake 3.10+

---

## 1. Architecture Overview

### 1.1 Design Goals
- **Hitch-Free Rendering**: Eliminate runtime PSO compilation stalls through aggressive caching
- **Persistent Caching**: Disk-based PSO storage with binary serialization (similar to UE5's `.stable.upipelinecache`)
- **Async Compilation**: Background PSO compilation with priority queues
- **Stable Keys**: Deterministic PSO hashing for reproducible cache hits across sessions
- **Memory Management**: Automatic cleanup of transient PSO data post-compilation

### 1.2 Core Components

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │  PSO Precache│  │ PSO FileCache│  │   RHI Command    │  │
│  │   Manager    │  │   Manager    │  │   List Interface │  │
│  └──────┬───────┘  └──────┬───────┘  └────────┬─────────┘  │
│         │                 │                    │            │
├─────────┴─────────────────┴────────────────────┴────────────┤
│              RHI Abstraction Layer (NVRHI)                  │
│  ┌───────────────────────────────────────────────────────┐  │
│  │  IDevice → createGraphicsPipeline/createComputePipeline│  │
│  │  ICommandList → setGraphicsState/setComputeState       │  │
│  │  IPipeline (Graphics/Compute/Meshlet/RayTracing)       │  │
│  └─────────────────────────┬─────────────────────────────┘  │
│                            │                                │
├────────────────────────────┼────────────────────────────────┤
│                    Vulkan Backend (NVRHI)                   │
│  ┌─────────────────────────┴─────────────────────────────┐  │
│  │  VkPipelineCache (driver-level)                       │  │
│  │  VkPipeline (monolithic PSOs)                         │  │
│  │  RenderPass compatibility management                  │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. Data Structures

### 2.1 PSO Initializer (Key Structure)

```cpp
// Base for all PSO types - must be hashable and comparable
struct FGraphicsPipelineStateInitializer {
    // Shaders
    nvrhi::ShaderHandle VertexShader;
    nvrhi::ShaderHandle PixelShader;
    nvrhi::ShaderHandle DomainShader;  // Optional (tessellation)
    nvrhi::ShaderHandle HullShader;    // Optional (tessellation)
    nvrhi::ShaderHandle GeometryShader; // Optional
    
    // Vertex Layout
    nvrhi::VertexAttributeDesc VertexAttributes[nvrhi::c_MaxVertexAttributes];
    uint32_t NumVertexAttributes;
    nvrhi::PrimitiveType PrimitiveType;
    
    // Render States
    nvrhi::RenderState RenderState;  // Contains BlendState, DepthStencilState, RasterState
    
    // Output Formats
    nvrhi::Format RenderTargetFormats[8];
    uint32_t NumRenderTargets;
    nvrhi::Format DepthStencilFormat;
    uint32_t SampleCount;
    uint32_t SampleQuality;
    
    // Binding Layouts
    nvrhi::BindingLayoutHandle BindingLayouts[16];
    uint32_t NumBindingLayouts;
    
    // UE5-specific: Stable shader keys for deterministic hashing
    FSHAHash VertexShaderHash;
    FSHAHash PixelShaderHash;
    // ... etc for other stages
    
    // Comparison operators for map keys
    bool operator==(const FGraphicsPipelineStateInitializer& Other) const;
    bool operator<(const FGraphicsPipelineStateInitializer& Other) const;
};

// Compute variant (simpler)
struct FComputePipelineStateInitializer {
    nvrhi::ShaderHandle ComputeShader;
    nvrhi::BindingLayoutHandle BindingLayouts[16];
    uint32_t NumBindingLayouts;
    FSHAHash ComputeShaderHash;
    
    bool operator==(const FComputePipelineStateInitializer& Other) const;
};
```

### 2.2 Stable Shader Keys (UE5 Pattern)

```cpp
// Deterministic hashing to survive shader recompilation
struct FStableShaderKey {
    FString ShaderType;           // "Vertex", "Pixel", "Compute"
    FString ShaderFile;           // Source file path (virtual)
    FString EntryPoint;           // Main/Entry function name
    FSHAHash SourceHash;          // Hash of source code
    FSHAHash CompiledHash;        // Hash of compiled bytecode
    uint32_t PermutationId;       // Shader variation ID
    
    // Serialization for disk cache
    void Serialize(FArchive& Ar);
};
```

### 2.3 Cached PSO Entry

```cpp
struct FCachedPSO {
    // Identification
    FGraphicsPipelineStateInitializer Initializer;
    FStableShaderKey StableKeys[5];  // Per shader stage
    
    // Runtime data (transient)
    nvrhi::GraphicsPipelineHandle Pipeline;
    std::atomic<bool> bIsCompiled{false};
    std::atomic<bool> bIsCompilationFailed{false};
    
    // Statistics
    uint64_t LastUsedFrame;
    uint32_t UseCount;
    
    // Binary cache data (for disk serialization)
    TArray<uint8_t> BinaryData;  // VkPipelineCache data
};
```

### 2.4 File Cache Format (UE5-compatible)

```cpp
// File header for .stable.upipelinecache equivalent
struct FPipelineCacheFileHeader {
    uint32_t Magic = 0x50434F53;  // 'PSOC'
    uint32_t Version = 1;
    FGuid CacheGuid;              // Unique per game build
    FGuid EngineGuid;             // UE5 version identifier
    uint64_t Timestamp;
    uint32_t Platform;
    uint32_t NumPSOs;
};

// Individual PSO entry in file
struct FPipelineCacheFileFormatPSO {
    enum class Type : uint32_t { Graphics = 0, Compute = 1 };
    
    Type PSOType;
    FGraphicsPipelineStateInitializer GraphicsDesc;  // Or ComputeDesc
    TArray<uint8_t> BinaryBlob;  // Driver-specific compiled data
    uint64_t UsageMask;          // Frequency hints
};
```

---

## 3. Core Systems

### 3.1 PSO Precache Manager (Async Compilation)

```cpp
class FPrecachePipelineManager {
public:
    // Initialize with thread pool for async compilation
    void Initialize(nvrhi::IDevice* Device, int32_t NumThreads = 4);
    
    // Request PSO precache (non-blocking)
    void PrecacheGraphicsPSO(const FGraphicsPipelineStateInitializer& Initializer);
    void PrecacheComputePSO(const FComputePipelineStateInitializer& Initializer);
    
    // Batch precache (level load optimization)
    void PrecachePSOArray(const TArray<FGraphicsPipelineStateInitializer>& Initializers);
    
    // Check if specific PSO is ready
    bool IsPSOReady(const FGraphicsPipelineStateInitializer& Initializer) const;
    
    // Wait for specific PSO (blocking, use sparingly)
    void WaitForPSO(const FGraphicsPipelineStateInitializer& Initializer, uint32_t TimeoutMs = 1000);
    
    // Frame update - cleanup completed tasks
    void Tick();
    
    // Memory management - release precached PSOs after timeout
    void SetPrecacheRetentionTime(float Seconds);
    
private:
    struct FPrecacheTask {
        FGraphicsPipelineStateInitializer Initializer;
        std::future<nvrhi::GraphicsPipelineHandle> Future;
        uint64_t RequestFrame;
    };
    
    nvrhi::IDevice* Device;
    std::unordered_map<size_t, FPrecacheTask> PendingTasks;
    std::unordered_map<size_t, FCachedPSO> CompletedPSOs;
    
    // Thread pool for compilation
    std::unique_ptr<FThreadPool> CompilationPool;
    
    // Hash function for initializers
    size_t HashInitializer(const FGraphicsPipelineStateInitializer& Init) const;
};
```

### 3.2 PSO File Cache Manager (Persistent Storage)

```cpp
class FPipelineFileCacheManager {
public:
    // Load cache from disk on startup
    bool OpenPipelineFileCache(const FString& CacheName, const FGuid& GameGuid);
    
    // Save cache to disk on shutdown
    bool SavePipelineFileCache();
    
    // Query if PSO exists in file cache
    bool HasPSO(const FGraphicsPipelineStateInitializer& Initializer) const;
    
    // Retrieve binary data for driver cache
    bool GetPSOBinaryData(const FGraphicsPipelineStateInitializer& Initializer, TArray<uint8_t>& OutData) const;
    
    // Add new PSO to cache (called after runtime compilation)
    void CacheGraphicsPSO(const FGraphicsPipelineStateInitializer& Initializer, const TArray<uint8_t>& BinaryData);
    void CacheComputePSO(const FComputePipelineStateInitializer& Initializer, const TArray<uint8_t>& BinaryData);
    
    // Merge new PSOs from current session into cache
    void MergeNewPSOs();
    
    // UE5: Expand stable PSO cache (cook-time operation)
    static bool BuildStablePSOCache(const FString& RecordingFilesDir, const FString& OutputPath);
    
private:
    FString CurrentCachePath;
    FGuid CurrentGameGuid;
    
    // In-memory representation of file
    TMap<FString, TUniquePtr<FPipelineCacheFile>> FileCacheMap;
    TSet<FPipelineCacheFileFormatPSO> NewPSOs;  // Unsaved PSOs from current session
    
    // Runtime lookup map
    TMap<size_t, FPipelineCacheFileFormatPSO> PSOMap;
};
```

### 3.3 Pipeline State Cache (Runtime)

```cpp
class FPipelineStateCache {
public:
    // Singleton access
    static FPipelineStateCache& Get();
    
    // Initialize RHI device reference
    void Initialize(nvrhi::IDevice* InDevice);
    
    // Get or create graphics pipeline (blocking, with fallback)
    nvrhi::GraphicsPipelineHandle GetOrCreateGraphicsPipeline(
        const FGraphicsPipelineStateInitializer& Initializer,
        bool bPrecache = false);
    
    nvrhi::ComputePipelineHandle GetOrCreateComputePipeline(
        const FComputePipelineStateInitializer& Initializer,
        bool bPrecache = false);
    
    // Find existing without creation
    nvrhi::GraphicsPipelineHandle FindGraphicsPipeline(const FGraphicsPipelineStateInitializer& Initializer) const;
    
    // Eviction policy
    void SetMaxCacheSize(uint32_t MaxPSOs);
    void EvictLeastRecentlyUsed(uint32_t Count);
    
    // Statistics
    struct FStats {
        uint32_t NumCachedPSOs;
        uint32_t NumCacheHits;
        uint32_t NumCacheMisses;
        uint32_t NumPrecachedPSOs;
        double AverageCompileTimeMs;
    };
    FStats GetStats() const;
    
private:
    nvrhi::IDevice* Device;
    
    // Thread-safe cache storage
    mutable std::shared_mutex CacheMutex;
    std::unordered_map<size_t, FCachedPSO> GraphicsPSOCache;
    std::unordered_map<size_t, FCachedPSO> ComputePSOCache;
    
    // LRU tracking
    std::list<size_t> LRUList;
    std::unordered_map<size_t, std::list<size_t>::iterator> LRUMap;
    
    // Sub-managers
    TUniquePtr<FPrecachePipelineManager> PrecacheManager;
    TUniquePtr<FPipelineFileCacheManager> FileCacheManager;
    
    // Internal creation (calls NVRHI)
    nvrhi::GraphicsPipelineHandle CreateGraphicsPipelineInternal(
        const FGraphicsPipelineStateInitializer& Initializer);
};
```

---

## 4. NVRHI Vulkan-Specific Implementation

### 4.1 Device Initialization (Linux Vulkan)

```cpp
class FVulkanRHI {
public:
    bool Initialize() {
        // NVRHI device creation for Linux Vulkan
        nvrhi::vulkan::DeviceDesc Desc;
        
        // Instance extensions
        Desc.requiredInstanceExtensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME,  // Or VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        };
        
        // Device extensions
        Desc.requiredDeviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_MAINTENANCE1_EXTENSION_NAME,
            VK_KHR_MAINTENANCE2_EXTENSION_NAME,
            VK_KHR_MAINTENANCE3_EXTENSION_NAME,
            // Pipeline cache control
            VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME,
            VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME
        };
        
        // Features
        Desc.enableRayTracing = false;  // Focus on graphics/compute first
        Desc.enableMeshShaders = false;
        
        // Create device
        Device = nvrhi::vulkan::createDevice(Desc);
        
        // Initialize PSO cache
        PipelineStateCache = MakeUnique<FPipelineStateCache>();
        PipelineStateCache->Initialize(Device);
        
        // Load file cache if exists
        FileCache = MakeUnique<FPipelineFileCacheManager>();
        FileCache->OpenPipelineFileCache(GameName, GameGuid);
        
        return true;
    }
    
private:
    nvrhi::DeviceHandle Device;
    TUniquePtr<FPipelineStateCache> PipelineStateCache;
    TUniquePtr<FPipelineFileCacheManager> FileCache;
};
```

### 4.2 Vulkan Pipeline Binary Extraction

```cpp
// Extract binary data for disk caching (Vulkan-specific)
TArray<uint8_t> FVulkanPipelineState::ExtractBinaryData() {
    if (!Pipeline) return {};
    
    // Access underlying VkPipeline
    VkPipeline VkPipelineHandle = static_cast<VkPipeline>(Pipeline->getNativeObject(nvrhi::ObjectTypes::VK_Pipeline));
    
    // Get pipeline cache data
    size_t DataSize = 0;
    vkGetPipelineCacheData(VkDevice, PipelineCache, &DataSize, nullptr);
    
    TArray<uint8_t> Data;
    Data.SetNumUninitialized(DataSize);
    vkGetPipelineCacheData(VkDevice, PipelineCache, &DataSize, Data.GetData());
    
    return Data;
}

// Load from binary data (warm start)
bool FVulkanPipelineState::LoadFromBinaryData(const TArray<uint8_t>& Data) {
    VkPipelineCacheCreateInfo CacheInfo = {};
    CacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    CacheInfo.initialDataSize = Data.Num();
    CacheInfo.pInitialData = Data.GetData();
    
    VkResult Result = vkCreatePipelineCache(VkDevice, &CacheInfo, nullptr, &PipelineCache);
    return Result == VK_SUCCESS;
}
```

---

## 5. Workflow Integration

### 5.1 Startup Sequence

```cpp
void FEngine::InitRHI() {
    // 1. Create NVRHI Vulkan device
    RHI = MakeUnique<FVulkanRHI>();
    RHI->Initialize();
    
    // 2. Load stable PSO cache (blocking, but fast - just deserializing)
    auto& PSOCache = FPipelineStateCache::Get();
    PSOCache.LoadStableCache(StableCachePath);
    
    // 3. Begin async precache of all known PSOs
    TArray<FGraphicsPipelineStateInitializer> KnownPSOs;
    LoadKnownPSOsFromStableCache(KnownPSOs);
    PSOCache.PrecachePSOArray(KnownPSOs);
    
    // 4. Optional: Show loading screen while critical PSOs compile
    WaitForCriticalPSOs();
}
```

### 5.2 Runtime Usage (Material/Mesh Rendering)

```cpp
void FMeshDrawCommand::Submit(FRHICommandList& CmdList) {
    // Build initializer from material + mesh + shader permutation
    FGraphicsPipelineStateInitializer PSOInit;
    BuildPSOInitializer(Material, Mesh, ShaderPermutation, PSOInit);
    
    // Get or create pipeline (fast path: cache hit)
    auto Pipeline = FPipelineStateCache::Get().GetOrCreateGraphicsPipeline(PSOInit);
    
    // Set state via NVRHI
    nvrhi::GraphicsState State;
    State.pipeline = Pipeline;
    State.framebuffer = CurrentFramebuffer;
    State.vertexBuffers = {VertexBuffer};
    State.indexBuffer = IndexBuffer;
    // ... other state
    
    CmdList->setGraphicsState(State);
    CmdList->drawIndexed({NumIndices, 1, 0, 0, 0});
}
```

### 5.3 Shutdown Sequence

```cpp
void FEngine::ShutdownRHI() {
    // 1. Save runtime-generated PSOs to file cache
    FPipelineStateCache::Get().SaveFileCache();
    
    // 2. Merge new PSOs into stable cache for next build
    FPipelineFileCacheManager::BuildStablePSOCache(
        RecordingDir, 
        OutputStableCachePath
    );
    
    // 3. Cleanup
    RHI->Shutdown();
}
```

---

## 6. Thread Safety & Synchronization

### 6.1 Compilation Threading

```cpp
// Thread-safe PSO creation
class FCompilePipelineStateTask {
public:
    void Execute() {
        // Create in background thread
        nvrhi::GraphicsPipelineHandle Pipeline = Device->createGraphicsPipeline(PSODesc);
        
        // Store result (thread-safe)
        std::lock_guard<std::mutex> Lock(Manager->CacheMutex);
        Manager->CompletedPSOs[Hash] = {Pipeline, true};
    }
    
private:
    nvrhi::IDevice* Device;  // NVRHI devices are thread-safe for creation
    FGraphicsPipelineStateInitializer PSODesc;
    size_t Hash;
    FPrecachePipelineManager* Manager;
};
```

### 6.2 Command List Safety

```cpp
// NVRHI command lists are NOT thread-safe for recording
// Use per-thread command lists for parallel PSO creation testing

void FParallelPSOCompiler::WorkerThread(int32 ThreadId) {
    // Each thread gets its own command list
    nvrhi::CommandListHandle CmdList = Device->createCommandList();
    CmdList->open();
    
    // Test-compile PSO by creating it
    auto Pipeline = Device->createGraphicsPipeline(PSODesc);
    
    // Optional: Draw a dummy triangle to fully warm the pipeline
    CmdList->setGraphicsState({Pipeline, DummyFramebuffer});
    CmdList->draw({3, 1, 0, 0, 0});
    
    CmdList->close();
    Device->executeCommandList(CmdList);
}
```

---

## 7. Build System (CMake)

```cmake
cmake_minimum_required(VERSION 3.10)
project(PSOCacheRHI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find NVRHI (assumed installed or submodule)
find_package(nvrhi REQUIRED)

# Find Vulkan
find_package(Vulkan REQUIRED)

# PSO Cache Library
add_library(PSOCacheRHI STATIC
    # Core
    PipelineStateCache.cpp
    PrecachePipelineManager.cpp
    PipelineFileCacheManager.cpp
    
    # Platform
    Vulkan/VulkanPipelineState.cpp
    Vulkan/VulkanRHIDevice.cpp
    
    # Serialization
    PSOArchive.cpp
    StableShaderKey.cpp
    
    # Utils
    PSOHash.cpp
    ThreadPool.cpp
)

target_link_libraries(PSOCacheRHI
    PUBLIC
        nvrhi::nvrhi
        nvrhi::nvrhi_vulkan
        Vulkan::Vulkan
        pthread  # Linux threading
)

target_compile_definitions(PSOCacheRHI
    PUBLIC
        NVRHI_WITH_VULKAN=1
        PLATFORM_LINUX=1
        USE_PSO_PRECACHING=1
        USE_PSO_FILE_CACHE=1
)

# Executable
add_executable(TestPSOCache main.cpp)
target_link_libraries(TestPSOCache PSOCacheRHI)
```

---

## 8. Testing Strategy

### 8.1 Unit Tests

```cpp
TEST(PSOCacheTest, StableHashConsistency) {
    FGraphicsPipelineStateInitializer Init1 = CreateTestPSO();
    FGraphicsPipelineStateInitializer Init2 = CreateTestPSO();
    
    size_t Hash1 = HashInitializer(Init1);
    size_t Hash2 = HashInitializer(Init2);
    
    EXPECT_EQ(Hash1, Hash2);  // Deterministic
}

TEST(PSOCacheTest, AsyncPrecache) {
    FPrecachePipelineManager Manager;
    Manager.Initialize(Device);
    
    auto Init = CreateComplexPSO();
    Manager.PrecacheGraphicsPSO(Init);
    
    // Wait for completion
    while (!Manager.IsPSOReady(Init)) {
        std::this_thread::sleep_for(10ms);
    }
    
    EXPECT_TRUE(Manager.IsPSOReady(Init));
}
```

### 8.2 Performance Benchmarks

```cpp
void BenchmarkPSOCompilation() {
    // Cold cache
    auto Start = FPlatformTime::Cycles();
    auto Pipeline = Cache.GetOrCreateGraphicsPipeline(Init, false);
    auto ColdTime = FPlatformTime::ToMilliseconds(FPlatformTime::Cycles() - Start);
    
    // Warm cache
    Start = FPlatformTime::Cycles();
    Pipeline = Cache.GetOrCreateGraphicsPipeline(Init, false);
    auto WarmTime = FPlatformTime::ToMilliseconds(FPlatformTime::Cycles() - Start);
    
    LOG_INFO("Cold: %.2fms, Warm: %.2fms, Speedup: %.1fx", 
             ColdTime, WarmTime, ColdTime/WarmTime);
    // Expect: Cold > 50ms, Warm < 0.1ms
}
```

---

## 9. UE5 Parity Checklist

| Feature | UE5 Implementation | This Framework | Priority |
|---------|-------------------|----------------|----------|
| **PSO Precache** | `FPrecacheGraphicsPipelineCache` | `FPrecachePipelineManager` | Critical |
| **File Cache** | `FPipelineFileCacheManager` | Same name | Critical |
| **Stable Keys** | `FStableShaderKeyIndex` | `FStableShaderKey` | Critical |
| **Binary Cache** | `D3D12PipelineLibrary` / `VK PipelineCache` | Vulkan PipelineCache | Critical |
| **Async Compile** | `FCompilePipelineStateTask` | `FCompilePipelineStateTask` | Critical |
| **PSO Recording** | `FShaderPipelineCache::SetPipelineStateUsage` | `FPipelineFileCacheManager::CacheGraphicsPSO` | High |
| **Retention Policy** | `r.PSOPrecache.BackgroundTaskWaitTime` | `SetPrecacheRetentionTime` | Medium |
| **Memory Budget** | `r.PSOPrecache.MaxPrecompiledPSOs` | `SetMaxCacheSize` | Medium |
| **Shader Library** | `FShaderLibrary` integration | Manual shader management | Low |

---

## 10. References

1. **NVRHI Documentation**: https://github.com/NVIDIA-RTX/NVRHI/tree/main/doc
2. **UE5 PSO Cache Docs**: https://dev.epicgames.com/documentation/en-us/unreal-engine/optimizing-rendering-with-pso-caches-in-unreal-engine
3. **Vulkan Pipeline Cache**: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPipelineCache.html
4. **UE5 Source**: `Engine/Source/Runtime/RHI/Public/PipelineStateCache.h`
5. **NVRHI Examples**: https://github.com/NVIDIA-RTX/NVRHI-Samples

---

## 11. Implementation Notes for LLM Agent

### Critical Implementation Details:

1. **Hash Function**: Must use stable hash (e.g., CityHash64) on shader bytecode + state description. Do NOT use pointer values.

2. **NVRHI Vulkan Specifics**: 
   - Use `nvrhi::vulkan::createDevice()` not D3D12
   - Pipeline creation requires `GraphicsPipelineDesc` filled completely
   - Render passes are automatic in NVRHI but affect PSO compatibility

3. **Linux Paths**: Use `$HOME/.local/share/[GameName]/PSOCache/` for user cache, `/var/cache/[GameName]/` for system-wide stable cache.

4. **Threading**: NVRHI device is thread-safe for resource creation, but command lists are NOT. Use multiple command lists for parallel PSO warm-up.

5. **Memory**: UE5 deletes RHI-level PSOs after precache to save memory. Keep only the Vulkan PipelineCache binary data, not the full `nvrhi::IPipeline` objects.

6. **Error Handling**: If PSO compilation fails (e.g., shader variant missing), fall back to default PSO and log error. Do NOT crash.

7. **Validation**: Enable NVRHI validation layer in debug builds: `nvrhi::enableDebugLayer()`.

**Start with**: `FPipelineStateCache` class and `FGraphicsPipelineStateInitializer` structure, then add `FPrecachePipelineManager` for async functionality.
```

This specification provides a complete blueprint for implementing a UE5-like PSO caching system using NVRHI with Vulkan on Linux. The architecture mirrors UE5's separation between Precache (async runtime compilation) and FileCache (persistent storage), adapted for NVRHI's abstraction layer.
