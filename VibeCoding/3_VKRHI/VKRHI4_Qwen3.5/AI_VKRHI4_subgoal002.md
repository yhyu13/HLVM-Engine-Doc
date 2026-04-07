# Subgoal 2: Implement PipelineState RHI Object

## Overview
Implement graphics and compute pipeline state objects that encapsulate shader binding, vertex input layout, rasterizer state, and other pipeline configuration.

## Complexity: Medium-High
This is a foundational object that builds on ShaderModule but requires understanding of NVRHI's pipeline state API.

## Design

### FPipelineState
Wrapper around `nvrhi::GraphicsPipelineState` and `nvrhi::ComputePipelineState`.

**Responsibilities**:
- Shader binding (vertex, fragment, compute, etc.)
- Vertex input layout definition
- Primitive topology (triangle list, line list, etc.)
- Rasterizer state (culling, depth bias, etc.)
- Depth/stencil state
- Blend state for color targets
- Pipeline layout (descriptor set layouts)

### FPipelineStateBuilder
Fluent builder pattern for constructing pipeline states.

**Rationale**: Pipeline state creation requires many parameters. Builder pattern provides:
- Type-safe construction
- Default values for optional parameters
- Clear required vs optional parameters
- Easier to read and maintain

## File Structure

### Public Headers
1. **PipelineState.h**
   - `FPipelineState` - Base class or variant class
   - `FPipelineStateBuilder` - Builder for graphics pipelines
   - `FComputePipelineStateBuilder` - Builder for compute pipelines
   - `FVertexInputLayout` - Vertex attribute descriptions
   - `FRasterizerState` - Rasterization configuration
   - `FDepthStencilState` - Depth/stencil configuration
   - `FBlendState` - Blend configuration per render target

### Private Implementations
1. **PipelineState.cpp**
   - Implementation of builders
   - Pipeline state creation logic
   - Validation of pipeline configuration

## Key NVRHI APIs to Use
- `nvrhi::GraphicsPipelineStateDesc`
- `nvrhi::ComputePipelineStateDesc`
- `nvrhi::VertexAttribute`
- `nvrhi::InputSlotDescription`
- `nvrhi::RasterState`
- `nvrhi::DepthStencilState`
- `nvrhi::BlendState`
- `nvrhi::ShaderHandle`
- `Device->createGraphicsPipeline()`
- `Device->createComputePipeline()`

## Implementation Plan

### Step 1: Basic Pipeline State
```cpp
class FPipelineState
{
public:
    NOCOPYMOVE(FPipelineState)
    
    FPipelineState();
    ~FPipelineState();
    
    [[nodiscard]] nvrhi::PipelineHandle GetPipelineHandle() const;
    [[nodiscard]] bool IsValid() const;
    
protected:
    nvrhi::PipelineHandle PipelineHandle;
    nvrhi::IDevice* Device;
};
```

### Step 2: Builder Pattern
```cpp
class FGraphicsPipelineBuilder
{
public:
    FGraphicsPipelineBuilder& SetDevice(nvrhi::IDevice* Device);
    FGraphicsPipelineBuilder& AddShader(nvrhi::ShaderHandle Shader, nvrhi::ShaderType Type);
    FGraphicsPipelineBuilder& SetVertexInputLayout(const FVertexInputLayout& Layout);
    FGraphicsPipelineBuilder& SetPrimitiveTopology(nvrhi::PrimitiveTopology Topology);
    FGraphicsPipelineBuilder& SetRasterizerState(const FRasterizerState& State);
    FGraphicsPipelineBuilder& SetDepthStencilState(const FDepthStencilState& State);
    FGraphicsPipelineBuilder& AddBlendState(const FBlendState& State);
    FGraphicsPipelineBuilder& SetFramebuffer(nvrhi::FramebufferHandle Framebuffer);
    
    TUniquePtr<FPipelineState> Build();
    
private:
    nvrhi::GraphicsPipelineStateDesc Desc;
    nvrhi::IDevice* Device = nullptr;
};
```

### Step 3: Helper Structures
```cpp
struct FVertexAttribute
{
    FString SemanticName;
    uint32_t SemanticIndex;
    nvrhi::Format Format;
    uint32_t BufferSlot;
    uint32_t Offset;
};

struct FVertexInputLayout
{
    TArray<FVertexAttribute> Attributes;
    uint32_t Stride;
    nvrhi::InputRate InputRate;
};

struct FRasterizerState
{
    bool bEnableDepthBias = false;
    float DepthBiasConstant = 0.0f;
    float DepthBiasSlope = 0.0f;
    nvrhi::CullMode CullMode = nvrhi::CullMode::Back;
    nvrhi::FrontCounterClockwise FrontCCW = nvrhi::FrontCounterClockwise::False;
    // ... more fields
};
```

## Success Criteria
- [ ] Can create graphics pipeline with vertex+fragment shaders
- [ ] Can create compute pipeline with compute shader
- [ ] Vertex input layout correctly describes buffer bindings
- [ ] Rasterizer state controls culling and depth bias
- [ ] Depth/stencil state controls depth testing
- [ ] Blend state controls color blending
- [ ] Builder pattern is intuitive and type-safe
- [ ] No compilation errors or warnings
- [ ] Proper error handling for invalid configurations

## Dependencies
- ShaderModule (for shader handles)
- NVRHI pipeline API
- Understanding of Vulkan graphics pipeline

## Testing Strategy
- Create simple triangle pipeline (vertex + fragment)
- Create compute pipeline (compute shader only)
- Verify pipeline creation fails gracefully with invalid inputs
- Test different vertex input layouts

## Notes
- Pipeline state creation is expensive - cache and reuse
- Some states may need to be immutable after creation
- Consider pipeline variant generation for different shader permutations
