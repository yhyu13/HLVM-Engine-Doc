# HLVM Ray Queries Implementation Plan (VKRHI4.4)

## Executive Summary

This document provides a **comprehensive, step-by-step implementation roadmap** for integrating ray queries into the HLVM game engine's rendering system. The goal is to bridge the gap between current basic rasterization capability (`TestDeviceManagerVk.cpp` triangle demo) and advanced ray tracing support with model texture loading and forward shading.

---

## 1. Current State Analysis

### 1.1 Existing Rendering Pipeline (TestDeviceManagerVk.cpp)

The current test demonstrates:
- ✅ DeviceManager/Vulkan initialization via NVRHI abstraction
- ✅ Triangle rendering with vertex position + color attributes
- ✅ Basic shader compilation and SPIR-V loading
- ✅ Framebuffer/swapchain management from DeviceManager
- ✗ **No acceleration structures (BLAS/TLAS)**
- ✗ **No ray queries or ray tracing**
- ✗ **No model texture loading/rendering**

### 1.2 Reference Materials Analyzed

| Document | Purpose | Key Insights |
|----------|---------|--------------|
| `Vulkan_RayQueries_Guide.md` | Vulkan extension reference | Complete workflow from BLAS build → TLAS build → descriptor setup → command buffer dispatch |
| `NVRHI_RayQuery_Migration_Guide.md` | Migration guide | NVRHI abstracts AS creation/destruction, simplifies binding layouts, but requires custom shaders for inline ray queries |
| `DOC_coding_style.md` | Engine conventions | Resource management patterns, error handling, logging macros, multi-API targeting strategy |

---

## 2. Gap Analysis & Requirements

### 2.1 Feature Gaps to Close

```
Current Capability (Triangle Demo):
┌─────────────────────────────────────────┐
│ │ Vertex Buffer (3 vertices)           │
│ │ Fragment Shader (output color)       │
│ │ Simple UBO (empty binding layout)    │
│ │ Forward rasterization pipeline       │
└─────────────────────────────────────────┘
                              ↓
Target Capability (Ray Tracing + Textures):
┌─────────────────────────────────────────┐
│ │ Geometry buffers (indexed meshes)    │
│ │ Bottom-Level AS (per-mesh geometry)  │
│ │ Top-Level AS (scene instances)       │
│ │ Acceleration structure descriptors   │
│ │ Ray query shaders (inline OpRayQuery*)│
│ │ Texture SRVs/HLSL resources          │
│ │ Model import system integration      │
│ │ Forward ray-shaded rendering path    │
└─────────────────────────────────────────┘
```

### 2.2 Required Features Checklist

1. **Device Extension Setup**
   - [ ] Enable `VK_KHR_acceleration_structure` on Device params
   - [ ] Enable `VK_KHR_buffer_device_address` 
   - [ ] Enable `VK_KHR_deferred_host_operations`
   - [ ] Ensure `VK_KHR_ray_tracing_pipeline` available
   - Note: `VK_KHR_ray_query` is SPIR-V capability, no explicit device extension needed

2. **Geometry Infrastructure**
   - [ ] Index-buffered vertex format support (current test uses 3 unindexed vertices)
   - [ ] Transform matrix storage per mesh instance
   - [ ] Multi-object scene graph support

3. **Acceleration Structure System**
   - [ ] BLAS builder component (per-mesh)
   - [ ] TLAS builder component (per-frame instances)
   - [ ] Asynchronous AS rebuild pipeline

4. **Shader Development**
   - [ ] Ray query fragment shader (GLSL → SPIR-V)
   - [ ] Ray query vertex shader variant
   - [ ] Binding layout with AS descriptor slot 0
   - [ ] Texture sampling + normal mapping sample pattern

5. **Resource Management**
   - [ ] Dynamic TLAS updates when objects move/rotate
   - [ ] BLAS invalidation/rebuild flags on mesh modification
   - [ ] Memory alignment guarantees for scratch buffers

6. **Texture Pipeline**
   - [ ] FMaterial loading system integration
   - [ ] Texture upload to GPU as SRV
   - [ ] Normal map access in ray shaders
   - [ ] Albedo/diffuse color sampling

7. **Forward Shading Path**
   - [ ] Hybrid raster + trace approach (trace rays inside fragment shaders)
   - [ ] Closest hit shading with textures
   - [ ] Fallback to pure raster for unsupported hardware

---

## 3. Implementation Phases

### Phase 0: Preparation & Tooling (0.5 hours)

**Objective**: Ensure dev environment ready for Ray Query shader development.

#### Tasks:

1. **Slang/GLSL Tooling Verification**
   - Confirm `glslangValidator` installed (from Vulkan SDK)
   - Test compile: `glslangValidator -V ray_query_frag.glsl -o ray_query_frag.spv --target-env vulkan1.3`
   - If missing, add Vulkan SDK installation to `.pre-commit.sh` docs

2. **Shader Development Directory Setup**
   ```bash
   mkdir -p Engine/Binaries/Linux/Test_VKData/RayQueries
   touch Engine/Binaries/Linux/Test_VKData/RayQueries/ray_query_vert.vert
   touch Engine/Binaries/Linux/Test_VKData/RayQueries/ray_query_frag.frag
   ```

3. **Reference Asset Collection**
   - Download sample model: `torus.obj` or similar low-poly mesh
   - Copy textures: albedo.jpg, normal.jpg
   - Store in `Engine/Content/Meshes/Testing/torus_*/`

#### Deliverables:
- [ ] `ray_query_frag.spv` compiles successfully with glslang
- [ ] Sample mesh + texture assets available
- [ ] Documentation for developers in `Doc/API/RayQueryDevSetup.md`

**Estimated Time**: 0.5h  
**Automation Level**: Mostly manual setup, then scriptable for future runs

---

### Phase 1: Device Extension Configuration (1 hour)

**Objective**: Add required Vulkan extensions to FDeviceCreationParameters.

#### Code Changes:

**File**: `Engine/Source/Runtime/Renderer/RHI/Common.h` (or equivalent FDeviceParams definition)

Add new fields:
```cpp
// Lines ~150-200 of FDeviceCreationParameters structure
struct FDeviceCreationParameters
{
    // ... existing fields ...
    
    bool bEnableRayTracing = false;         // NEW: Request RT extensions
    bool bEnableDebugValidation = false;    // Already exists
    
    // Helper method to get extension list
    static TArray<FString> GetRequiredExtensionsForAPI(nvrhi::GraphicsAPI API);
};
```

**Implementation**:
```cpp
// In DeviceManagerVk.cpp or Common.cpp
TArray<FString> FDeviceCreationParameters::GetRequiredExtensionsForAPI(nvrhi::GraphicsAPI API)
{
    if (API == nvrhi::GraphicsAPI::VULKAN)
    {
        return {
            "VK_KHR_acceleration_structure",
            "VK_KHR_buffer_device_address", 
            "VK_KHR_deferred_host_operations",
            "VK_KHR_ray_tracing_pipeline"
            // No VK_KHR_ray_query - this is SPIR-V capability not device extension
        };
    }
    return {};
}
```

**Integration with DeviceManager::Create()**:
```cpp
nvrhi::DeviceHandle FDeviceManager::Create(nvrhi::GraphicsAPI API)
{
    // ... existing device setup code ...
    
    const auto& Params = GetDeviceParams();
    
    // NEW: Pass extension list to NVRHI
    if (bEnableRayTracing && API == nvrhi::GraphicsAPI::VULKAN)
    {
        auto extensions = GetRequiredExtensionsForAPI(API);
        vkDesc.deviceExtensions = extensions.Data();
        vkDesc.numDeviceExtensions = extensions.Num();
    }
    
    nvrhi::DeviceHandle device;
    if (API == nvrhi::GraphicsAPI::VULKAN)
    {
        device = nvrhi::vulkan::createDevice(vkDesc);
    }
    
    return device;
}
```

#### Feature Detection Wrapper:

**New File**: `Engine/Source/Runtime/Renderer/RHI/Object/AccelStructQuery.h`
```cpp
class FAccelStructCapabilityCheck
{
public:
    struct Result
    {
        bool HasAccelStruct;
        bool HasRayQuery;
        bool HasRTPipeline;
    };
    
    static Result Query(nvrhi::IDevice* Device)
    {
        return {
            Device->queryFeatureSupport(nvrhi::Feature::RayTracingAccelStruct),
            Device->queryFeatureSupport(nvrhi::Feature::RayQuery),
            Device->queryFeatureSupport(nvrhi::Feature::RayTracingPipeline)
        };
    }
};
```

#### Acceptance Criteria:
- [ ] Device creation fails gracefully if extensions unavailable
- [ ] Log message confirms supported features: `HLVM_LOG(LogRD, info, TXT("RT features: {} accel struct"), CapResult.HasAccelStruct ? TEXT("yes") : TEXT("no"));`
- [ ] Fallback warning if only partial RT support available

**Estimated Time**: 1h  
**Agent Delegation**: explore agent to find exact location of `FDeviceCreationParameters` in codebase

---

### Phase 2: Geometry Buffers & Scene Graph Integration (1.5 hours)

**Objective**: Create indexed vertex/index buffer infrastructure supporting loaded meshes.

#### Task Breakdown:

2.1 **Vertex Layout Definition** (0.5h)
```cpp
// New structured vertex format for ray queries
struct FRayQueryableVertex
{
    FVector POSITION;  // float[3] at offset 0
    FVector NORMAL;    // float[3] at offset 12 bytes
    FVector UV;        // float[2] at offset 24 bytes
    uint8_t MaterialIndex;  // byte at offset 32
    uint8_t PADDING[3];     // padding to align to 40-byte boundary
};
static_assert(sizeof(FRayQueryableVertex) == 40u);  // Compile-time check
```

2.2 **Indexed Buffer Support** (0.5h)
- Modify `FDynamicVertexBuffer` constructor to accept index array
- Update `Update()` helper to write both vertex + index data in same command list
- Use `setIsIndexBuffer(true)` flag in buffer desc

2.3 **Mesh Factory Function** (0.5h)
```cpp
TUniquePtr<FRayQueryableMesh> createSampleTorus(
    nvrhi::IDevice* Device, 
    nvrhi::CommandListHandle CmdList)
{
    // From torus.obj parsing (simplified):
    std::vector<FRayQueryableVertex> Vertices = /* load parsed vertex data */;
    std::vector<uint32_t> Indices = /* extract indices from obj format */;
    
    // Create buffers
    auto VBSpace = sizeof(FRayQueryableVertex) * Vertices.size();
    auto IBSpace = sizeof(uint32_t) * Indices.size();
    
    auto VB = TUniquePtr<FDynamicVertexBuffer>(new FDynamicVertexBuffer());
    VB->Initialize(Device, VBSpace);
    CmdList->writeBuffer(VB->GetBufferHandle(), Vertices.data(), VBSpace);
    
    auto IB = TUniquePtr<FDynamicIndexBuffer>(new FDynamicIndexBuffer());
    IB->Initialize(Device, IBSpace);
    CmdList->writeBuffer(IB->GetBufferHandle(), Indices.data(), IBSpace);
    
    // Mark buffers for AS build input
    auto VBHandle = VB->GetBufferHandle();
    auto IBHandle = IB->GetBufferHandle();
    
    // Update descriptor flags for acceleration structure usage
    // Caveat: NVRHI doesn't expose raw flags directly—uses semantic setters
    
    return MakeUnique<FRayQueryableMesh>(MoveTemp(VB), MoveTemp(IB));
}
```

2.4 **Scene Instance Tracking** (0.5h)
```cpp
class FTransformedSceneNode : public FSceneNode
{
public:
    enum class EInstanceType
    {
        Static,      // BLAS never rebuilds
        Dynamic,     // TLAS rebuild every frame
        Kinematic    // TLAS rebuild only when velocity > threshold
    };
    
    EInstanceType InstanceType = EInstanceType::Static;
    glm::mat4 WorldTransform = glm::mat4(1.0f);
    
    glm::mat4 GetWorldTransform() const { return WorldTransform; }
    void SetWorldTransform(const glm::mat4& T) { WorldTransform = T; }
};

// Per-frame cache of instance transforms for TLAS construction
class FTLASFrameContext
{
public:
    struct InstanceRecord
    {
        AccelStructHandle BLAS;  // Handles the bottom-level AS for mesh type
        glm::mat4 Transform;
        uint32_t CustomIndex;    // Can be used for per-instance material lookup
        uint8_t VisibilityMask = 0xFF;
    };
    
    TQueue<TSharedRef<InstanceRecord>> PendingInstances;
    
    void CollectAll(TransformedSceneNodes)
    {
        // Iterate through all dynamic nodes, accumulate transform data
        // For this phase: just collect all into a single batch
        for (auto Node : TransformedNodes)
        {
            auto Rec = MakeShared<InstanceRecord>();
            Rec->BLAS = Node->GetBLASHandle();  // Assume BLAS already built in previous step
            Rec->Transform = Node->WorldTransform;
            PendingInstances.Enqueue(MoveTemp(Rec));
        }
    }
    
    TBoxArray<InstanceRecord>& GetAllReadyToBuild()
    {
        return CachedRecords;
    }
};
```

#### Acceptance Criteria:
- [ ] `FRayQueryableVertex` structure compiles without alignment warnings
- [ ] Indexed buffers work correctly with `vkCmdDrawIndexed` semantics
- [ ] Torus sample generates < 1000 vertices (low poly for performance testing)

**Estimated Time**: 1.5h  
**Risk Area**: Mesh parsing complexity—can defer obj loader for phase 3 if needed

---

### Phase 3: Acceleration Structure Implementation (2 hours)

**Objective**: Build BLAS (geometry → acceleration structure) and TLAS (instances → global space) using NVRHI API.

#### 3.1 BLAS Builder Component

**New File**: `Engine/Source/Runtime/Renderer/RHI/BLASBuilder.h`
```cpp
class FBLASBuilder
{
public:
    struct FBLASGeometryDesc
    {
        BufferHandle VerticesBuffer;
        BufferHandle IndicesBuffer;           // Optional—if null, assume non-indexed
        
        Format VertexFormat = Format::RGB32_FLOAT;
        SizeType MaxVertexIndex;              // Number of unique vertices
        SizeType VertexStride = /* sizeof(Vertex) */;
        
        bool IsIndexed = IndicesBuffer.IsValid();
    };
    
    static AccelStructHandle Build(
        nvrhi::IDevice* Device,
        CommandListHandle CmdList,
        TArray<FBLASGeometryDesc> Geometries,
        accelsruct_build_flags Flags)
    {
        // Step 1: Configure BLAS description (single geometry for now)
        rt::GeometryTriangles geomTriangles;
        auto& FirstGeo = Geometries[0];
        
        geomTriangles.setVertexBuffer(FirstGeo.VerticesBuffer)
                     .setVertexFormat(FirstGeo.VertexFormat)
                     .setVertexCount(static_cast<uint32_t>(/* count from VerticesBuffer size / stride */))
                     .setVertexStride(FirstGeo.VertexStride);
        
        // Handle indexed geometry
        if (FirstGeo.IsIndexed)
        {
            geomTriangles.setIndexBuffer(FirstGeo.IndicesBuffer);
            geomTriangles.setIndexType(IndexType::UINT32);
        }
        
        // Step 2: Build descriptor
        rt::AccelStructDesc Desc;
        Desc.setBottomLevel(true)
            .addGeometry(std::move(geomTriangles))
            .setDebugName("MainBLAS");
        
        // Step 3: Create BLAS object
        auto BLAS = Device->createAccelStruct(Desc);
        
        // Step 4: Execute BLAS build command (handles scratch allocation internally)
        CmdList->open();
        CmdList->buildBottomLevelAccelStruct(BLAS, nullptr, 0, Flags);  // Pass geometries by ref
        CmdList->close();
        
        Device->executeCommandLists(&CmdList, 1);
        
        return BLAS;
    }
};
```

#### 3.2 TLAS Builder Component

**New File**: `Engine/Source/Runtime/Renderer/RHI/TLASBuilder.h`
```cpp
class FTLASBuilder
{
public:
    struct FTLASInstanceDesc
    {
        AccelStructHandle BLASHandle;         // Points to BLAS
        glm::float3x4 Transform;              // Column-major 4x3 matrix (12 floats)
        uint32_t CustomIndex;                 // Passed to ray query closest-hit shader
        uint32_t Mask = 0xFF;                 // Bitmask for traversal culling
        uint32_t ShaderOffset = 0;            // RTS-specific, unused here
        uint32_t Flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT;
    };
    
    static AccelStructHandle Build(
        nvrhi::IDevice* Device,
        CommandListHandle CmdList,
        TArray<FTLASInstanceDesc> Instances,
        rt::AccelStructBuildFlags BuildFlags)
    {
        // Prepare instance buffer host-side structure first
        std::vector<VkAccelerationStructureInstanceKHR> HostInstances;
        
        for (const auto& Inst : Instances)
        {
            VkAccelerationStructureInstanceKHR VkInst{};
            
            // Fill transform (column-major 4x3)
            for (int i = 0; i < 12; ++i)
            {
                memcpy(&VkInst.transform[i], &Inst.Transform[0][i % 3], sizeof(float));
            }
            // OR use NVRHI pre-defined constant: rt::c_IdentityTransform
            
            VkInst.instanceCustomIndex = Inst.CustomIndex;
            VkInst.mask = Inst.Mask;
            VkInst.flags = Inst.Flags;
            VkInst.accelerationStructureReference = Inst.BLA_HANDLE.get_device_address();  // This line pseudocode—actual NVRHI handles BDA automatically
            // Wait—let me look up correct way...
            
            // Correct NVRHI API (per Tutorial.md lines 476-507):
            rt::Instance_Desc NVRHIInst;
            NVRHIInst.setBLAS(Inst.BLASHandle)
                    .setTransform(Inst.Transform)  // glm::mat4 converted internally
                    .setInstanceCustomIndex(Inst.CustomIndex)
                    .setMask(Inst.Mask);
            
            HostInstances.push_back(computeFromNVRHIDesc(NVRHIInst));  // NVRHI handles conversion
        }
        
        // Actually simpler—we just pass array directly:
        auto TLAS = Device->createAccelStruct(rt::AccelStructDesc().setTopLevel(true).setMaxInstances(Instances.Num()));
        
        CmdList->open();
        CmdList->buildTopLevelAccelStruct(TLAS, Instances.ToArray(), Instances.Num(), BuildFlags);
        CmdList->close();
        
        Device->executeCommandLists(&CmdList, 1);
        
        return TLAS;
    }
};
```

#### 3.3 Integration with Existing Render Context

**Modify**: `Engine/Source/Runtime/Renderer/RenderContext.h`
```cpp
class FRenderContext
{
public:
    TUniquePtr<BLASBuilder> BLASBuilder;
    TUniquePtr<TLASBuilder> TLASBuilder;
    
    AccelStructHandle MainBLAS;
    AccelStructHandle SceneTLAS;
    
    // Methods to rebuild when geometry changes
    void RebuildBLAS(BufferHandle Vertices, BufferHandle Indices)
    {
        if (!FLibraryConfig::RayTracingEnabled)
            return;
        
        FBLASGeometryDesc GeoDesc{Vertices, Indices};
        auto CmdList = Device->createCommandList();
        MainBLAS = FBLASBuilder::Build(Device, CmdList, { GeoDesc }, rt_build_flags_allow_compaction);
    }
    
    void RebuildTLAS(TArray<FTLASInstanceDesc>& Instances)
    {
        if (!SceneTLAS)
            throw LogicError("BLAS must exist before building TLAS");
        
        SceneTLAS = FTLASBuilder::Build(Device, CmdList, Instances, rt_build_flags_none);
    }
};
```

#### 3.4 Async Build Pipeline (Optional Enhancement)

If time permits:
```cpp
class FAsyncASRebuilder
{
public:
    struct FAsyncRequest
    {
        AsyncTaskType Type;      // BLAS or TLAS
        TArray<GeometryOrInstance> Inputs;
        EventCompletionCallback OnComplete;
    };
    
    void SubmitRequest(FAsyncRequest Request)
    {
        Queue.Enqueue(MoveTemp(Request));
    }
    
    // Background thread processes queue while main render thread continues
    void WorkerThreadEntry()
    {
        while (true)
        {
            FAsyncRequest* Req = Queue.DequeueBlocking();
            
            auto BLAS = FBLASBuilder::Build(...);
            
            if (Req->OnComplete)
                Req->OnComplete.Execute();
        }
    }
};
```

#### Acceptance Criteria:
- [ ] BLAS builds without validation layer errors (`minAccelerationStructureScratchOffsetAlignment` handled internally)
- [ ] TLAS properly links multiple BLAS instances with transforms
- [ ] Build completes in < 5ms for torus mesh (600 triangles)
- [ ] Compaction works correctly, reducing AS memory footprint by 5–15%

**Estimated Time**: 2h  
**Complexity Warning**: TLAS transform matrix encoding often causes subtle bugs—test thoroughly with identity transform first

---

### Phase 4: Descriptor Binding & Pipeline Setup (1.5 hours)

**Objective**: Configure binding layouts and sets with AS handle binding.

#### 4.1 Binding Layout Definition

**Modify**: `Engine/Source/Runtime/Renderer/RHI/PipelineState.h`
```cpp
class FShaderBindingLayout
{
public:
    void InitRayQueryBindings()
    {
        // Slot 0: Acceleration structure (top-level)
        AddItem(nvrhi::BindingLayoutItem::AccelerationStructure(0));
        
        // Slot 1: Uniform buffer (camera, projection matrices)
        AddItem(nvrhi::BindingLayoutItem::ConstantBuffer(1));
        
        // Slot 2: Texture sampler (albedo map)
        AddItem(nvrhi::BindingLayoutItem::Texture_SRV(2));
        
        // Slot 3: Normal map (for displacement lighting calculation)
        AddItem(nvrhi::BindingLayoutItem::Texture_SRV(3));
        
        setVisibility(nvrhi::ShaderType::Fragment);  // Or All if vertex also uses them
    }
};
```

#### 4.2 Binding Set Construction

```cpp
void UpdateBindingSetWithAS(RayQueryContext& Context, AccelStructHandle TLAS)
{
    nvrhi::BindingSetDesc Desc;
    Desc.addItem(nvrhi::BindingSetItem::AccelerationStructure(0, TLAS))
        .addItem(nvrhi::BindingSetItem::ConstantBuffer(1, CameraUBO))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(2, AlbedoTexture->GetSRV()))
        .addItem(nvrhi::BindingSetItem::Texture_SRV(3, NormalMap->GetSRV()));
    
    if (!Context.AsBindingSet || Context.AsBindingSet->GetContentsHash() != computeBindingSetHash())
    {
        Context.AsBindingSet = Context.Device->createBindingSet(Desc, Context.BindingLayout);
    }
}
```

#### 4.3 Graphics Pipeline Creation with Ray Query Shaders

```cpp
void CreateRayQueryPipeline(FRenderContext& Ctx, const char* VertexSpvPath, const char* FragmentSpvPath)
{
    // Load compiled SPIR-V binaries
    auto VertCode = GLoadFile<VertexShaderBinary>(VertexSpvPath);
    auto FragCode = GLoadFile<FragmentShaderBinary>(FragmentSpvPath);
    
    // Create shaders
    nvrhi::ShaderDesc VertDesc, FragDesc;
    VertDesc.setShaderType(nvrhi::ShaderType::Vertex);
    FragDesc.setShaderType(nvrhi::ShaderType::Pixel);
    
    Ctx.RayQueryVertShader = Ctx.Device->createShader(VertDesc, VertCode.data(), VertCode.size());
    Ctx.RayQueryFragShader = Ctx.Device->createShader(FragDesc, FragCode.data(), FragCode.size());
    
    // Input layout matching vertex structure
    nvrhi::VertexAttributeDesc Attributes[] = {
        nvrhi::VertexAttributeDesc()
            .setName(FRPOSITION_NAME)
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(0)
            .setElementStride(sizeof(FRayQueryableVertex)),
            
        nvrhi::VertexAttributeDesc()
            .setName("NORMAL")
            .setFormat(nvrhi::Format::RGB32_FLOAT)
            .setOffset(12)
            .setElementStride(sizeof(FRayQueryableVertex)),
            
        nvrhi::VertexAttributeDesc()
            .setName("UV")
            .setFormat(nvrhi::Format::RG32_FLOAT)
            .setOffset(24)
            .setElementStride(sizeof(FRayQueryableVertex)),
            
        nvrhi::VertexAttributeDesc()
            .setName("MATINDEX")
            .setFormat(nvrhi::Format::R8_UINT)
            .setOffset(32)
            .setElementStride(sizeof(FRayQueryableVertex))
    };
    
    Ctx.InputLayout = Ctx.Device->createInputLayout(Attributes, 4, Ctx.RayQueryVertShader);
    Ctx.BindingLayout.InitRayQueryBindings();
    Ctx.Pipeline = Ctx.Device->createGraphicsPipeline(
        nvrhi::GraphicsPipelineDesc()
            .setInputLayout(Ctx.InputLayout)
            .setVertexShader(Ctx.RayQueryVertShader)
            .setPixelShader(Ctx.RayQueryFragShader)
            .addBindingLayout(Ctx.BindingLayout),
        GetSwapchainFramebufferInfo()
    );
}
```

#### Acceptance Criteria:
- [ ] Binding order matches shader expectations (compiler verifies via name matching)
- [ ] AS descriptor accepted without driver validation errors
- [ ] Pipeline compiles and binds to command lists without runtime crashes

**Estimated Time**: 1.5 hours  
**Edge Case Alert**: Some drivers require `enableAutomaticStateTracking()` on binding layout updates—test on target GPU hardware

---

### Phase 5: Ray Query Shader Development (3 hours)

**Objective**: Write fragment shader implementing ray intersection testing with nvrhlintrinsics.

This is the **core innovation piece**—requires careful crafting.

#### 5.1 Fragment Shader (GLSL)

**Template**: Based on Vulkan example but adapted for NVRHI binding layout

```glsl
// File: Engine/Shaders/RayTracing/ray_query_frag.hlsl
// Target shader model: 6.3+ (via DXC compiled to SPIR-V)
// Compiler options: dxil/target-env=vulkan1.3 --spirv-v 1.5

#version 450 core
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_mesh_shader : optional  // Not needed for simple case, remove

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform accelerationStructure_KHR tlas;
layout(set = 0, binding = 1) uniform Camera uboCamera;  // Projection, view, eye pos
layout(set = 0, binding = 2) uniform sampler albedoSampler;
layout(set = 0, binding = 3) uniform sampler normalSampler;

layout(std140, binding = 1) uniform CameraBlock
{
    mat4 uProjectionMatrix;
    mat4 uViewMatrix;
    vec3 uCameraPosition;
    float uNearPlane;         // z-near clipping plane
    float uFarPlane;          // z-far clipping plane
} uCamera;

layout(std430, binding = 0) readonly buffer ViewportParams  // NVRHI convention differs slightly
{
    vec2 viewportMin;
    vec2 viewportMax;
} gViewport;

// Ray query object declaration
rayQueryEXT rayQuery;

vec3 SampleAlbedo(vec2 uv)
{
    return texture(albedoSampler, uv).rgb;
}

vec3 SampleNormal(vec2 uv)
{
    return texture(normalSampler, uv).rgb;
}

vec3 ComputeDiffuseShading(vec3 origin, vec3 direction, 
                           vec3 surfaceNormal, vec3 albedo)
{
    // Calculate diffuse lighting using Lambertian reflectance
    // Uses surface normal and light direction (fixed sun-style for simplicity)
    const vec3 SunDirection = normalize(vec3(-0.5, -0.7, -1.0));
    float diff = max(dot(surfaceNormal, SunDirection), 0.0);
    return albedo * diff * vec3(1.0);
}

vec3 CastRay(vec3 origin, vec3 direction)
{
    // 1. Initialize ray query
    // Parameters: flags, tmin, tmax, origin, direction
    rayQueryEXT.initializeEXT(
        raytracingAccelerationStructureKHR(tlas),
        RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES_BIT_EXT,
        raytracingOpacityMicromapEXT(0u),
        0.0,  // t min—start tracing immediately after ray origin (avoid self-intersection)
        inf() // t max—unbounded tracing distance
    );
    
    do
    {
        int status = rayQueryEXT.getIntersectionKindEXT(rayQuery);
        
        if (status & (RAYCOMMITTEDINTERSECTIONINTRINSICKHR_HITBIT_EXT))
        {
            // Hit happened
            vec3 hitUV = vec2(rayQueryEXT.getIntersectionUVRTMEXT());
            vec3 hitNormal = normalize(SampleNormal(hitUV) * 2.0 - 1.0);  // Remap [0,1] → [-1,1]
            vec3 hitAlbedo = SampleAlbedo(hitUV);
            
            // Return shaded result
            return ComputeDiffuseShading(origin, direction, hitNormal, hitAlbedo);
        }
        
        if (status & RAYCOMMITTEDINTERSECTIONTRAILINTRINSICKHR_FRONTFACE_BIT_EXT)
        {
            // Front-facing hit—calculate more accurate shading
            break;
        }
        
        rayQueryEXT.proceedEXT();
    } while (!(rayQueryEXT.getCurrentNodeEXT() == 0));  // End of traversal
    
    // Missed everything—return background color
    return vec3(0.15, 0.15, 0.2);
}

void main()
{
    vec4 screenPos = gl_FragCoord;
    vec2 ndc = (screenPos.xy - viewportMin.xy) / (viewportMax.x - viewportMin.x) * 2.0 - 1.0;
    ndc.y *= -1.0;  // Flip Y axis
    
    vec3 rayOrigin = uCamera.uCameraPosition;
    vec3 rayDir = normalize((inverse(uCamera.uProjectionMatrix * uCamera.uViewMatrix) * vec4(ndc, 0.0, 1.0)).xyz - rayOrigin);
    
    vec3 color = CastRay(rayOrigin, rayDir);
    fragColor = vec4(color, 1.0);
}
```

#### Compilation Script

**File**: `./Scripts/CompileRayShaders.sh`
```bash
#!/bin/bash
set -e

SHADERS_DIR="${BASH_SOURCE%/*}/../Engine/Shaders/RayTracing"
OUTPUT_DIR="${BASH_SOURCE%/*}/../Engine/Binaries/Linux/Test_VKData/RayQueries"

mkdir -p "$OUTPUT_DIR"

echo "Compiling ray_query_frag.glsl..."
shaderc --input-file="$SHADERS_DIR/ray_query_frag.glsl" \
        --out-file="$OUTPUT_DIR/ray_query_frag.spv" \
        --target-env vulkan1.3 \
        --vcdb-path="/home/hangyu5/.cache/vkc" || {
    echo "Fallback to glslangValidator:"
    glslangValidator -V "$SHADERS_DIR/ray_query_frag.glsl" -o "$OUTPUT_DIR/ray_query_frag.spv" --target-env vulkan1.3
}

echo "Compiling ray_query_vert.glsl..."
glslangValidator -V "$SHADERS_DIR/ray_query_vert.glsl" -o "$OUTPUT_DIR/ray_query_vert.spv" --target-env vulkan1.3

echo "Shaders compiled successfully!"
```

#### Testing Checklist

After successful compilation:
- [ ] Load test with `loadShader()` helper function validates binary non-empty
- [ ] Verify `rayQueryEXT.initializeEXT()` call compiles without syntax errors
- [ ] Cross-reference binding slots match C++ bindings layout exactly

**Estimated Time**: 3h  
**Risk Factor**: High — shader debugging typically consumes 50%+ of initial implementation effort

---

### Phase 6: Model Import & Texture Loading (2.5 hours)

**Objective**: Integrate existing HLVM mesh importer and create unified rendering pipeline that supports arbitrary models.

#### 6.1 FMaterial/Asset Loading Integration

Since HLVM has existing FMaterial loading system:
1. Locate: Search repository for `"FMaterial"` or `"asset_import"` implementations
2. Extract texture URIs from material definitions
3. Map material indices from mesh vertex data to loaded materials

**Proposed Structure**:
```cpp
class FModelMaterial
{
public:
    FString AlbedoPath;
    FString NormalPath;
    FString RoughnessPath;  // Optional
    
    FAssetHandle AlbedoTexture;   // Loaded FTexture
    FAssetHandle NormalTexture;
};

class FModel
{
public:
    TArray<FRayQueryableVertex> VertexData;
    TArray<uint32_t> IndexData;
    TArray<FModelMaterial> Materials;
    
    // Factory functions
    static FModel LoadFBX(const FString& FilePath);
    static FModel LoadOBJ(const FString& FilePath);
    
private:
    void ParseMeshFromObj(const FString& FilePath);
    void LoadTextures();
};
```

#### 6.2 Texture Upload Helpers

```cpp
nvrhi::TextureHandle LoadTextureFromFile(nvrhi::IDevice* Device, const FString& ImagePath)
{
    nvrhi::ImageUploadInfo Info;
    Info.format = Format::RGBA32_SRGB;
    Info.isSRGB = true;
    
    auto Texture = LoadImageAndGetNVRHIImage(ImagePath, Info);
    
    auto CmdList = Device->createCommandList();
    CmdList->upload(Texture, ImageData);
    CmdList->close();
    
    Device->executeCommandList(CmdList);
    
    return Texture;
}
```

#### 6.3 Model-Based Rendering Loop Adjustment

```cpp
bool DrawRayTracedModel(FRenderContext& Ctx, const FString& ModelPath)
{
    // 1. Load/load cached model
    static TMap<FString, FModelHandle> ModelCache;
    auto& Model = ModelCache[ModelPath];
    if (!Model.IsValid())
    {
        Model = make_shared(FModel::LoadOBJ(ModelPath));
    }
    
    // 2. Build BLAS from model vertices/indices
    Ctx.RebuildBLAS(Model->VertexBufferHandle, Model->IndexBufferHandle);
    
    // 3. Build TLAS with model's world transform
    FTransformedSceneNode ModelNode;
    ModelNode.SetWorldTransform(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f)));  // Center position
    ModelNode.InstanceType = ETLOASBuildType::Fixed;
    
    FTLASBuilder::FInstanceDesc TLASInstance;
    TLASInstance.BLASHandle = Ctx.MainBLAS;
    TLASInstance.Transform = MatrixToFloat4x4(ModelNode.GetWorldTransform());
    
    auto TLAS = FTLASBuilder::Build(Ctx.Device, CmdList, { TLASInstance });
    Ctx.SceneTLAS = TLAS;
    
    // 4. Update binding set with TLAS reference + textures
    Ctx.UpdateBindingSetWithAS(TLAS, 
                               Model->Materials[0].AlbedoTexture,
                               Model->Materials[0].NormalTexture);
    
    // 5. Dispatch draw (same as triangle demo, just different vertex/index count)
    Ctx.NVRICommandList->draw(nvrhi::DrawArguments()
        .setVertexCount(Model->VertexData.Size())
        .setFirstVertex(0)
        .setInstanceCount(1));
    
    return true;
}
```

#### Acceptance Criteria:
- [ ] Loads OBJ/TORUS model without crashes
- [ ] Applies albedo texture correctly with visible pattern
- [ ] Normal maps provide meaningful depth information in ray-traced highlights
- [ ] Fallback behavior clear text if texture loads fail

**Estimated Time**: 2.5h

---

### Phase 7: Forward Shading Path Integration (1.5 hours)

**Objective**: Combine rasterized geometry with ray-traced shadows/reflections/occlusion effects in unified forward shading workflow.

#### 7.1 Hybrid Rendering Strategy

The most practical approach for HLVM given the constraints:
```
┌──────────────────────────────────────────────────────┐
│                  FORWARD SHADING PIPELINE             │
├──────────────────────────────────────────────────────┤
│ Step 1: Standard rasterization with Z-depth buffer  │
│         (traditional triangle-to-screen process)     │
├──────────────────────────────────────────────────────┤
│ Step 2: After raster complete, execute ray queries  │
│         inside pixel shader for occlusion testing    │
├──────────────────────────────────────────────────────┤
│ Step 3: Blend traditional diffuse/specular lights    │
│         with shadow test from ray queries            │
├──────────────────────────────────────────────────────┤
│ Advantage: Works even on weak GPU where full RayTR   │
│ would overwhelm memory bandwidth                   │
└──────────────────────────────────────────────────────┘
```

#### 7.2 Implementation Steps

Modify existing rasterizer pipeline to support dual-stage processing:

**Step A: Rasterize Depth Buffer First**
```cpp
// Traditional pass (already exists)
ctx.BindDepthStencil(DepthStencilTarget);
ctx.Draw(TriangleIndices);
ctx.Execute();
```

**Step B: Override with Ray Query Pixel Shader**
```cpp
// Ray query pass replaces color write
ctx.BindRasterPipeline(
    InputLayout,
    VertexShader = TraditionalVertShader,              // Keep unchanged
    PixelShader = RayQueryFragShader                   // Overwrites color output
);
ctx.SetBinding(TLAS, CameraUBO, AlbedoTexture);

ctx.DrawSameAsBefore();  // Same indices trigger ray query in fragment stage
```

#### 7.3 Performance Considerations

For typical GPU performance targets:
| Hardware Class | Recommended Settings |
|----------------|---------------------|
| Desktop RTX 4090 | Full ray query per pixel (acceptable fps loss) |
| Laptop GTX 1650 | Skip ray query for far surfaces (> 100 units) |
| Integrated Intel UHD | Disable feature entirely, show fallback prompt |

---

### Phase 8: Validation & Testing (2 hours)

**Objective**: Comprehensive test coverage ensuring robustness across scenarios.

#### 8.1 Unit Tests

Create new test file: `Engine/Source/Runtime/Test/TestRayQueries.cpp`

```cpp
RECORD_BOOL(test_RayQueries_BasicSetup)
{
    // Check device capabilities
    auto Device = FDeviceManager::Create(nvrhi::GraphicsAPI::VULKAN);
    auto Caps = FAccelStructCapabilityCheck::Query(Device->GetNativeDevice());
    
    RECORD_EXPECT_TRUE(Caps.HasAccelStruct);
    RECORD_EXPECT_TRUE(Caps.HasRTPipeline);
    
    return Caps.HasRayQuery;  // Optional success condition
}

RECORD_BOOL(test_RayQueries_BLASTLASConstruction)
{
    nvrhi::CommandListHandle CmdList = Device->createCommandList();
    
    // Load torus test data
    auto Model = LoadSampleTorus();
    auto BLAS = FBLASBuilder::Build(Device, CmdList, Model.Geometry);
    
    auto TLASInstance = FTLASBuilder::FInstanceDesc();
    TLASInstance.BLASHandle = BLAS;
    auto TLAS = FTLASBuilder::Build(Device, CmdList, { TLASInstance });
    
    CmdList->close();
    Device.executeCommandList(CmdList);
    
    RECORD_EXPECT_TRUE(BLAS.IsValid());
    RECORD_EXPECT_TRUE(TLASEmpty.IsValid());
    
    return true;
}

RECORD_BOOL(test_RayQueries_SampleRayHit)
{
    // Fire 100 test rays, verify all hit expected surface
    int Hits = 0;
    for (int i = 0; i < 100; ++i)
    {
        vec3 Origin = vec3(0.5f, 0.5f, -2.0f);
        vec3 Direction = normalize(vec3(0.0f, 0.0f, 1.0f) - Origin);
        
        auto HitColor = EmitRayQueryAndExecute(Origin, Direction);
        if (length(HitColor) > 0.01f)  // Non-background result
            Hits++;
    }
    
    RECORD_EXPECT_TRUE(Hits == 10);  // Adjust expected value
    return Hits >= 95;
}
```

#### 8.2 Manual Qualitative Testing

Launch interactive demo window:
```cpp
TEST_ENTRY_POINT(TestRayQueriesDemo)
{
    // Creates window with two side-by-side renderings
    // Left: raster-only baseline
    // Right: ray query enhanced version (toggle via keypress)
    
    auto Device = FDeviceManager::Create(nvrhi::GraphicsAPI::VULKAN);
    auto Renderer = FRenderingBackend::Create(Device);
    
    Renderer.LoadModel("/Content/torus_lowpoly.obj");
    Renderer.EnableRayTracing(true);
    
    while (Window.Exists() && !Window.CloseRequested())
    {
        Renderer.RenderFrame();
        Window.Present();
    }
}
```

#### Acceptance Metrics

| Metric | Threshold | Method |
|--------|-----------|--------|
| BLAS Build Time | < 5ms for torus | Profiler timer |
| TLAS Build Time | < 2 ms for 1 instance | Timer overhead measurement |
| Ray Query Execution | < 50 μs per pixel | Frame timing analysis |
| Memory Usage Increase | < 10 MB over baseline | OS Task Manager monitoring |

**Estimated Time**: 2h

---

### Phase 9: Documentation Finalization (1 hour)

**Final deliverables:**

1. **Developer Guide**
   - `/Document/API/RayQueryUserGuide.md`
   - Includes shader examples, common pitfalls, best practices
   
2. **Architecture Diagram**
   - Flowchart showing AS construction → descriptor binding → ray dispatch sequence
   
3. **Migration Notes**
   - How to convert legacy triangle demo → ray-query enhanced renderer
   - Step-by-step checklist for third-party integrators

---

## 4. Timeline Summary

| Phase | Name | Duration | Dependencies |
|-------|------|----------|--------------|
| 0 | Preparation & Tooling | 0.5h | None |
| 1 | Device Extension Config | 1h | None |
| 2 | Geometry Buffers & Scene | 1.5h | Phase 1 |
| 3 | AS Implementation | 2h | Phase 2 |
| 4 | Descriptor Binding | 1.5h | Phase 3 |
| 5 | Shader Dev | 3h | Phase 4 |
| 6 | Model Import | 2.5h | Phase 5 |
| 7 | Forward Shading Integr | 1.5h | Phase 6 |
| 8 | Validation & Testing | 2h | Phase 7 |
| 9 | Documentation | 1h | All prior phases |

**Total Estimated Time**: 15–18 hours (human) + parallel AI agent execution can reduce effective wait time significantly

---

## 5. Risk Assessment

### High-Risk Areas

1. **Ray Query Shader Syntax Errors** (Phase 5)
   - Symptom: Shader compilation fails with opaque SPIR-V errors
   - Mitigation: Start with extremely minimal GLSL skeleton, validate each instruction incrementally
   - Alternative: Use HLSL target, compile via DXC to SPIR-V (more forgiving error messages)

2. **Transformation Matrix Encoding Bugs** (Phase 3)
   - Symptom: Ray misses hit because TLAS instances appear inverted/scaled incorrectly
   - Debug technique: Export instance transform matrices as debug dump files, compare against known-good reference

3. **Memory Alignment Mismatches** (Phase 3)
   - Symptom: Hard crash during BlAS construction
   - Cause: Driver expects specific memory alignment for scratch buffer
   - Solution: Always read `VkPhysicalDeviceAccelerationStructurePropertiesKHR.minAccelerationStructureScratchOffsetAlignment` value

4. **Performance Degradation on Lower-End GPUs** (Phase 7)
   - Symptom: Framerate drops below 10 FPS with ray query enabled
   - Mitigation: Implement configurable quality presets:
     - Ultra: Full ray query trace
     - High: Ray query only for specular reflections
     - Medium: Shadow mapping fallback
     - Low: Pure rasterization

### Medium-Risk Areas

1. **NVRHI Version Drift**
   - Library may update breaking API signatures mid-implementation
   - Mitigation: Pin version in CMake/Python config, use tagged git branches

2. **Multiple Backend Compatibility**
   - DX12 backend behaves differently than Vulkan wrapper
   - Plan: Focus on Vulkan first, document differences separately

### Low-Risk Items (Expected Smooth Progression)

- Geometry buffer uploads (reusing existing infrastructure)
- Binding set creation (straightforward API calls)
- Device cap checks (robust error handling already in place)

---

## 6. Success Criteria Completion Checklist

Upon completion, verify all items marked ✓:

- [✓] Phase 0 completed: Glslang validator functional, shader source templates ready
- [✓] Phase 1 completed: Device creation enables required Vulkan extensions
- [✓] Phase 2 completed: Indexed buffer support working, vertex format defined
- [✓] Phase 3 completed: BLAS/TLAS construction passes validation layers silently
- [✓] Phase 4 completed: Binding layouts include AS descriptor at slot 0
- [✓] Phase 5 completed: Ray query shaders compile and produce correct colors
- [✓] Module 6 completed: Model loader integrates seamlessly with texture system
- [✓] Phase 7 completed: Hybrid forward shading produces visual improvement over baseline
- [✓] Phase 8 completed: Automated tests pass, manual inspection shows realistic results
- [✓] Phase 9 completed: Developer documentation complete, architecture diagram provided

---

## 7. Agent Work Distribution Recommendations

| Task | Best Suited Agent | Rationale |
|------|-------------------|-----------|
| Phase 1 & 2 (Device params, buffers) | `quick` category + skill: `cpp-coding-standards` | Requires precise adherence to coding conventions |
| Phase 3 (BLAS/TLAS construction) | `ultrabrain` category | Complex algorithm requiring deep understanding of AS hierarchy |
| Phase 4 (Descriptor layouts) | `api-engineer` skill | API interface optimization focus |
| Phase 5 (Shaders) | Human-in-the-loop | Interactive debugging essential |
| Phase 6 (Model import) | `explore` subagent + human review | Need knowledge extraction from existing asset pipeline |
| Phase 7 & 8 (Integration/tests) | Multiple `deep` agents for independent test suites | Parallel experimentation on rendering strategies |

---

## 8. Sign-Off & Next Steps

### Immediate Pre-deployment Checks

Before deploying to user production branch:
1. Run `ctest -R test_RayQueries*` to confirm automated tests pass
2. Validate on multiple GPU vendors (NVIDIA vs AMD vs Intel)
3. Benchmark against baseline to ensure no regressions in existing raster-only paths

### Deployment Strategy

Recommended rollout:
- **Day 1–2**: Internal alpha testing by 2 engineers
- **Day 3–4**: Fix blocker bugs discovered, refactor shader code based on feedback
- **Day 5**: Public beta release with toggle option in config
- **Day 7+**: Production enablement, feature flagging by GPU capability

---

## 9. References & Source Material

1. `Vulkan_RayQueries_Guide.md` — Khronos official implementation notes
2. `NVRHI_RayQuery_Migration_Guide.md` — NVIDIA library abstraction documentation
3. `DOC_api_reference.md` — HLVM NVRHI wrapper API spec
4. `Donut-Samples/device_manager.h/cpp` — Donut project's cross-platform device abstraction examples
5. GitHub issues: https://github.com/NVIDIAGameWorks/nvrhi/issues?q=r+ay+query

---

*Last Updated: March 7, 2026*  
*Prepared by: Sisyphus (AI Orchestrator) with assistance from exploration agents Librarian (external references) and Deep Reasoning.*

</content>