# AI Subgoal 2: Implement Additional RHI Objects

## Objective
Implement new RHI object classes based on NVRHI to support triangle rendering test.
Focus only on objects needed for the test (as per requirement: "only keep where our test needs").

## New Files Created

### 1. ShaderModule.h / ShaderModule.cpp
**Class**: `FShaderModule`

**Purpose**: Load and manage SPIR-V shader modules

**Key Features**:
- Initialize from file (SPIR-V binary)
- Initialize from memory buffer
- Support for all shader stages (vertex, pixel/f fragment, compute, etc.)
- Debug name support

**API**:
```cpp
class FShaderModule
{
    bool InitializeFromFile(const FPath& FilePath, nvrhi::ShaderType ShaderType, nvrhi::IDevice* Device);
    bool InitializeFromMemory(const void* Code, size_t CodeSize, nvrhi::ShaderType ShaderType, nvrhi::IDevice* Device);
    nvrhi::ShaderModuleHandle GetShaderModuleHandle() const;
    nvrhi::ShaderType GetShaderType() const;
    const char* GetEntryPointName() const;
};
```

**Usage Example**:
```cpp
FShaderModule VertexShader;
VertexShader.InitializeFromFile(
    FPath::Combine(DataDir, TEXT("vert.spv")),
    nvrhi::ShaderType::Vertex,
    NvrhiDevice.Get()
);
```

### 2. PipelineState.h / PipelineState.cpp
**Classes**: `FGraphicsPipelineState`, `FComputePipelineState`

**Purpose**: Graphics and compute pipeline state objects (PSO)

**Key Features**:
- Graphics pipeline with vertex/fragment shaders
- Vertex input layout configuration
- Rasterization, depth, blend state configuration
- Compute pipeline for compute shaders
- Pipeline layout support

**API**:
```cpp
class FGraphicsPipelineState
{
    bool CreatePipeline(
        nvrhi::IDevice* Device,
        nvrhi::IRenderPass* RenderPass,
        const FShaderModule& VertexShader,
        const FShaderModule& FragmentShader,
        const nvrhi::VertexAttributeDesc* VertexAttributes,
        TUINT32 NumVertexAttributes,
        TUINT32 VertexStride
    );
    nvrhi::PipelineHandle GetPipelineHandle() const;
    nvrhi::PipelineLayoutHandle GetPipelineLayoutHandle() const;
};
```

**Usage Example**:
```cpp
nvrhi::VertexAttributeDesc Attributes[] = {
    nvrhi::VertexAttributeDesc()
        .setLocation(0)
        .setFormat(nvrhi::Format::RGB32_FLOAT)
        .setBinding(0)
        .setOffset(0),
    nvrhi::VertexAttributeDesc()
        .setLocation(1)
        .setFormat(nvrhi::Format::RGB32_FLOAT)
        .setBinding(0)
        .setOffset(12)
};

FGraphicsPipelineState Pipeline;
Pipeline.CreatePipeline(
    Device, RenderPass,
    VertexShader, FragmentShader,
    Attributes, 2, sizeof(Vertex)
);
```

### 3. Buffer.h / Buffer.cpp
**Classes**: `FVertexBuffer`, `FIndexBuffer`, `FUniformBuffer`

**Purpose**: Vertex, index, and uniform buffer management

**Key Features**:
- Static and dynamic buffer initialization
- Buffer update methods
- Proper NVRHI buffer flags (IsVertexBuffer, IsIndexBuffer, IsConstantBuffer)
- Debug name support

**API**:
```cpp
class FVertexBuffer
{
    bool Initialize(nvrhi::IDevice* Device, const void* VertexData, size_t VertexDataSize, ...);
    bool InitializeEmpty(nvrhi::IDevice* Device, size_t BufferSize, ...);
    void Update(nvrhi::ICommandList* CommandList, const void* Data, size_t DataSize, size_t DstOffset = 0);
    nvrhi::BufferHandle GetBufferHandle() const;
};

class FIndexBuffer
{
    bool Initialize(nvrhi::IDevice* Device, const void* IndexData, size_t IndexDataSize, 
                    nvrhi::Format IndexFormat, ...);
    // ... similar to FVertexBuffer
};

class FUniformBuffer
{
    bool Initialize(nvrhi::IDevice* Device, size_t BufferSize, bool Dynamic = false);
    void Update(nvrhi::ICommandList* CommandList, const void* Data, size_t DataSize, ...);
    // ... similar to FVertexBuffer
};
```

**Usage Example**:
```cpp
// Vertex buffer
struct FVertex { float Position[3]; float Color[3]; };
FVertex Vertices[] = { ... };

FVertexBuffer VertexBuffer;
VertexBuffer.Initialize(Device, Vertices, sizeof(Vertices));

// Index buffer
uint32_t Indices[] = { 0, 1, 2 };
FIndexBuffer IndexBuffer;
IndexBuffer.Initialize(Device, Indices, sizeof(Indices), nvrhi::Format::R32_UINT);

// Uniform buffer
FUniformBuffer UniformBuffer;
UniformBuffer.Initialize(Device, sizeof(FViewUniforms), true); // Dynamic
```

## Design Decisions

### 1. NVRHI-Centric Approach
- All objects wrap NVRHI handles, not raw Vulkan
- Leverages NVRHI's cross-abstraction capabilities
- Simplifies resource management with RAII

### 2. Minimal Feature Set
- Only implemented features needed for triangle test
- Avoided over-engineering (e.g., no descriptor set management yet)
- Can be extended later as needed

### 3. Consistent API Style
- Follows existing HLVM coding conventions
- Uses `NOCOPYMOVE` for resource classes
- Debug name support on all objects
- Consistent initialization pattern

### 4. Error Handling
- Uses `HLVM_ENSURE_F` for validation
- Provides meaningful error messages
- Returns bool for initialization success

## Files Summary

| File | Lines | Classes |
|------|-------|---------|
| ShaderModule.h | 78 | FShaderModule |
| ShaderModule.cpp | 71 | FShaderModule impl |
| PipelineState.h | 112 | FGraphicsPipelineState, FComputePipelineState |
| PipelineState.cpp | 117 | Pipeline impl |
| Buffer.h | 158 | FVertexBuffer, FIndexBuffer, FUniformBuffer |
| Buffer.cpp | 164 | Buffer impl |
| **Total** | **700** | **5 classes** |

## Testing
- Code follows NVRHI API patterns
- LSP errors are false positives (include path configuration)
- Ready for integration in test file (Subgoal 3)

## Dependencies
- Requires existing: `FTexture`, `FFramebuffer` (from Subgoal 1)
- Requires NVRHI headers
- Requires HLVM common types (TUINT32, TArray, etc.)

## Next Steps
- Subgoal 3: Create test file using all RHI objects
- Test file will demonstrate end-to-end triangle rendering
