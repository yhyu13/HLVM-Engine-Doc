# NVRHI API Reference Guide

## Table of Contents
- [Overview](#overview)
- [Core Interfaces](#core-interfaces)
  - [IDevice](#idevice)
  - [ICommandList](#icommandlist)
  - [IResource](#iresource)
- [Resource Types](#resource-types)
  - [ITexture](#itexture)
  - [IBuffer](#ibuffer)
  - [IHeap](#iheap)
  - [ISampler](#isampler)
  - [IInputLayout](#iinputlayout)
  - [IFramebuffer](#iframebuffer)
- [Graphics Pipeline](#graphics-pipeline)
  - [Graphics State](#graphics-state)
  - [Blend State](#blend-state)
  - [Raster State](#raster-state)
  - [Depth Stencil State](#depth-stencil-state)
  - [Viewport State](#viewport-state)
- [Shader and Pipeline](#shader-and-pipeline)
  - [Shader Types](#shader-types)
  - [Graphics Pipeline](#graphics-pipeline-1)
  - [Compute Pipeline](#compute-pipeline)
  - [Meshlet Pipeline](#meshlet-pipeline)
- [Ray Tracing](#ray-tracing)
  - [Acceleration Structures](#acceleration-structures)
  - [Ray Tracing Pipeline](#ray-tracing-pipeline)
  - [Opacity Micromap](#opacity-micromap)
- [Resource Binding](#resource-binding)
  - [Binding Layouts](#binding-layouts)
  - [Binding Sets](#binding-sets)
  - [Descriptor Tables](#descriptor-tables)
- [Specialized Features](#specialized-features)
  - [Variable Rate Shading](#variable-rate-shading)
  - [Sampler Feedback](#sampler-feedback)
  - [Cooperative Vectors](#cooperative-vectors)
- [Query Objects](#query-objects)
  - [Event Queries](#event-queries)
  - [Timer Queries](#timer-queries)
- [Utility Types](#utility-types)
  - [Enums](#enums)
  - [Structs](#structs)
  - [Constants](#constants)
- [Backend-Specific Extensions](#backend-specific-extensions)
  - [Vulkan Extensions](#vulkan-extensions)
  - [DirectX 12 Extensions](#directx-12-extensions)
- [Best Practices](#best-practices)

## Overview

NVRHI (NVIDIA Rendering Hardware Interface) is a cross-platform graphics API abstraction library that provides a unified interface for DirectX 11, DirectX 12, and Vulkan 1.3. It works on Windows (x64 only) and Linux (x64 and ARM64).

### Key Features

- **Automatic resource state tracking** and barrier placement (optional)
- **Automatic resource lifetime management** with deferred and safe destruction
- **Efficient resource binding model** with minimal runtime overhead
- **Direct native API access** when needed via `getNativeObject()`
- **Cross-platform portability** with easy switching between graphics APIs
- **Hidden sub-allocation** of upload buffers and versioning of constant buffers
- **Parallel command list recording** and multi-queue rendering support
- **Full pipeline support**: Graphics, Compute, Ray Tracing, and Meshlet pipelines
- **Validation layer** and resource reflection for debugging
- **RTXMU integration** for acceleration structure memory management (optional)
- **NVAPI support** for DX11/DX12 extensions (optional)

### Architecture

NVRHI follows an interface-based design pattern where the main `IDevice` interface serves as the factory for all resources and command lists. Resources are reference-counted using COM-style `RefCountPtr` smart pointers.

```cpp
// Example: Creating a device and texture
nvrhi::DeviceHandle device = nvrhi::vulkan::createDevice(deviceDesc);
nvrhi::TextureHandle texture = device->createTexture(textureDesc);
```

### Header Version

The current API header version is 23. Use `verifyHeaderVersion()` to ensure compatibility when using NVRHI as a shared library:

```cpp
if (!nvrhi::verifyHeaderVersion()) {
    // Handle version mismatch
}
```

## Core Interfaces

### IDevice

The main device interface that serves as the factory for all resources and command lists. Provides device capabilities and format support queries.

**Header**: `include/nvrhi/nvrhi.h` (lines 3596-3714)

#### Key Methods

**Resource Creation:**
```cpp
virtual HeapHandle createHeap(const HeapDesc& d) = 0;
virtual TextureHandle createTexture(const TextureDesc& d) = 0;
virtual BufferHandle createBuffer(const BufferDesc& d) = 0;
virtual SamplerHandle createSampler(const SamplerDesc& d) = 0;
virtual ShaderHandle createShader(const ShaderDesc& d, const void* binary, size_t binarySize) = 0;
virtual ShaderLibraryHandle createShaderLibrary(const void* binary, size_t binarySize) = 0;
virtual InputLayoutHandle createInputLayout(const VertexAttributeDesc* d, uint32_t attributeCount, IShader* vertexShader) = 0;
```

**Pipeline Creation:**
```cpp
virtual GraphicsPipelineHandle createGraphicsPipeline(const GraphicsPipelineDesc& desc, FramebufferInfo const& fbinfo) = 0;
virtual ComputePipelineHandle createComputePipeline(const ComputePipelineDesc& desc) = 0;
virtual MeshletPipelineHandle createMeshletPipeline(const MeshletPipelineDesc& desc, FramebufferInfo const& fbinfo) = 0;
virtual rt::PipelineHandle createRayTracingPipeline(const rt::PipelineDesc& desc) = 0;
```

**Command List Management:**
```cpp
virtual CommandListHandle createCommandList(const CommandListParameters& params = CommandListParameters()) = 0;
virtual uint64_t executeCommandLists(ICommandList* const* pCommandLists, size_t numCommandLists, CommandQueue executionQueue = CommandQueue::Graphics) = 0;
virtual void queueWaitForCommandList(CommandQueue waitQueue, CommandQueue executionQueue, uint64_t instance) = 0;
virtual bool waitForIdle() = 0;
```

**Memory and Resource Binding:**
```cpp
virtual BindingLayoutHandle createBindingLayout(const BindingLayoutDesc& desc) = 0;
virtual BindingLayoutHandle createBindlessLayout(const BindlessLayoutDesc& desc) = 0;
virtual BindingSetHandle createBindingSet(const BindingSetDesc& desc, IBindingLayout* layout) = 0;
virtual DescriptorTableHandle createDescriptorTable(IBindingLayout* layout) = 0;
virtual void resizeDescriptorTable(IDescriptorTable* descriptorTable, uint32_t newSize, bool keepContents = true) = 0;
virtual bool writeDescriptorTable(IDescriptorTable* descriptorTable, const BindingSetItem& item) = 0;
```

**Ray Tracing:**
```cpp
virtual rt::OpacityMicromapHandle createOpacityMicromap(const rt::OpacityMicromapDesc& desc) = 0;
virtual rt::AccelStructHandle createAccelStruct(const rt::AccelStructDesc& desc) = 0;
virtual MemoryRequirements getAccelStructMemoryRequirements(rt::IAccelStruct* as) = 0;
```

**Query Objects:**
```cpp
virtual EventQueryHandle createEventQuery() = 0;
virtual void setEventQuery(IEventQuery* query, CommandQueue queue) = 0;
virtual bool pollEventQuery(IEventQuery* query) = 0;
virtual void waitEventQuery(IEventQuery* query) = 0;
virtual void resetEventQuery(IEventQuery* query) = 0;

virtual TimerQueryHandle createTimerQuery() = 0;
virtual bool pollTimerQuery(ITimerQuery* query) = 0;
virtual float getTimerQueryTime(ITimerQuery* query) = 0;
virtual void resetTimerQuery(ITimerQuery* query) = 0;
```

**Feature Support:**
```cpp
virtual bool queryFeatureSupport(Feature feature, void* pInfo = nullptr, size_t infoSize = 0) = 0;
virtual FormatSupport queryFormatSupport(Format format) = 0;
virtual GraphicsAPI getGraphicsAPI() = 0;
```

**Memory Management:**
```cpp
virtual void runGarbageCollection() = 0;  // IMPORTANT: Call at least once per frame
```

**Cooperative Vectors:**
```cpp
virtual coopvec::DeviceFeatures queryCoopVecFeatures() = 0;
virtual size_t getCoopVecMatrixSize(coopvec::DataType type, coopvec::MatrixLayout layout, int rows, int columns) = 0;
```

#### Usage Example

```cpp
// Create device
nvrhi::DeviceDesc deviceDesc;
deviceDesc.errorCB = &messageCallback;
// ... configure deviceDesc based on backend ...

nvrhi::DeviceHandle device = nvrhi::vulkan::createDevice(deviceDesc);
if (!device) {
    logError("Failed to create device");
    return;
}

// Check feature support
bool rtSupported = device->queryFeatureSupport(nvrhi::Feature::RayTracingPipeline);

// Create resources
nvrhi::TextureDesc textureDesc;
textureDesc.setWidth(1024)
    .setHeight(1024)
    .setFormat(nvrhi::Format::RGBA8_UNORM)
    .setIsRenderTarget(true)
    .enableAutomaticStateTracking(nvrhi::ResourceStates::RenderTarget)
    .setDebugName("MyRenderTarget");

nvrhi::TextureHandle renderTarget = device->createTexture(textureDesc);

// Create command list
nvrhi::CommandListHandle cmdList = device->createCommandList();
```

### ICommandList

Represents a sequence of GPU operations for recording and execution. Provides resource state management and automatic barrier placement.

**Header**: `include/nvrhi/nvrhi.h` (lines 3133-3586)

#### Key Methods

**Command List Management:**
```cpp
virtual void open() = 0;
virtual void close() = 0;
virtual void clearState() = 0;
```

**Rendering Commands:**
```cpp
virtual void draw(const DrawArguments& args) = 0;
virtual void drawIndexed(const DrawArguments& args) = 0;
virtual void drawIndirect(uint32_t offsetBytes, uint32_t drawCount = 1) = 0;
virtual void drawIndexedIndirect(uint32_t offsetBytes, uint32_t drawCount = 1) = 0;
virtual void drawIndexedIndirectCount(uint32_t paramOffsetBytes, uint32_t countOffsetBytes, uint32_t maxDrawCount) = 0;
```

**Compute Commands:**
```cpp
virtual void dispatch(uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1) = 0;
virtual void dispatchIndirect(uint32_t offsetBytes) = 0;
```

**Meshlet Commands:**
```cpp
virtual void setMeshletState(const MeshletState& state) = 0;
virtual void dispatchMesh(uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1) = 0;
```

**Ray Tracing Commands:**
```cpp
virtual void setRayTracingState(const rt::State& state) = 0;
virtual void dispatchRays(const rt::DispatchRaysArguments& args) = 0;
virtual void buildOpacityMicromap(rt::IOpacityMicromap* omm, const rt::OpacityMicromapDesc& desc) = 0;
virtual void buildBottomLevelAccelStruct(rt::IAccelStruct* as, const rt::GeometryDesc* pGeometries, size_t numGeometries, rt::AccelStructBuildFlags buildFlags = rt::AccelStructBuildFlags::None) = 0;
virtual void compactBottomLevelAccelStructs() = 0;
virtual void buildTopLevelAccelStruct(rt::IAccelStruct* as, const rt::InstanceDesc* pInstances, size_t numInstances, rt::AccelStructBuildFlags buildFlags = rt::AccelStructBuildFlags::None) = 0;
virtual void buildTopLevelAccelStructFromBuffer(rt::IAccelStruct* as, nvrhi::IBuffer* instanceBuffer, uint64_t instanceBufferOffset, size_t numInstances, rt::AccelStructBuildFlags buildFlags = rt::AccelStructBuildFlags::None) = 0;
virtual void executeMultiIndirectClusterOperation(const rt::cluster::OperationDesc& desc) = 0;
```

**Texture Operations:**
```cpp
virtual void clearTextureFloat(ITexture* t, TextureSubresourceSet subresources, const Color& clearColor) = 0;
virtual void clearDepthStencilTexture(ITexture* t, TextureSubresourceSet subresources, bool clearDepth, float depth, bool clearStencil, uint8_t stencil) = 0;
virtual void clearTextureUInt(ITexture* t, TextureSubresourceSet subresources, uint32_t clearColor) = 0;
virtual void copyTexture(ITexture* dest, const TextureSlice& destSlice, ITexture* src, const TextureSlice& srcSlice) = 0;
virtual void copyTexture(IStagingTexture* dest, const TextureSlice& destSlice, ITexture* src, const TextureSlice& srcSlice) = 0;
virtual void copyTexture(ITexture* dest, const TextureSlice& destSlice, IStagingTexture* src, const TextureSlice& srcSlice) = 0;
virtual void writeTexture(ITexture* dest, uint32_t arraySlice, uint32_t mipLevel, const void* data, size_t rowPitch, size_t depthPitch = 0) = 0;
virtual void resolveTexture(ITexture* dest, const TextureSubresourceSet& dstSubresources, ITexture* src, const TextureSubresourceSet& srcSubresources) = 0;
```

**Buffer Operations:**
```cpp
virtual void writeBuffer(IBuffer* b, const void* data, size_t dataSize, uint64_t destOffsetBytes = 0) = 0;
virtual void clearBufferUInt(IBuffer* b, uint32_t clearValue) = 0;
virtual void copyBuffer(IBuffer* dest, uint64_t destOffsetBytes, IBuffer* src, uint64_t srcOffsetBytes, uint64_t dataSizeBytes) = 0;
```

**Sampler Feedback:**
```cpp
virtual void clearSamplerFeedbackTexture(ISamplerFeedbackTexture* texture) = 0;
virtual void decodeSamplerFeedbackTexture(IBuffer* buffer, ISamplerFeedbackTexture* texture, nvrhi::Format format) = 0;
virtual void setSamplerFeedbackTextureState(ISamplerFeedbackTexture* texture, ResourceStates stateBits) = 0;
```

**State Setting:**
```cpp
virtual void setGraphicsState(const GraphicsState& state) = 0;
virtual void setComputeState(const ComputeState& state) = 0;
virtual void setPushConstants(const void* data, size_t byteSize) = 0;
```

**Resource State Tracking:**
```cpp
virtual void setEnableAutomaticBarriers(bool enable) = 0;
virtual void setResourceStatesForBindingSet(IBindingSet* bindingSet) = 0;
virtual void setResourceStatesForFramebuffer(IFramebuffer* framebuffer) = 0;
virtual void setEnableUavBarriersForTexture(ITexture* texture, bool enableBarriers) = 0;
virtual void setEnableUavBarriersForBuffer(IBuffer* buffer, bool enableBarriers) = 0;
virtual void beginTrackingTextureState(ITexture* texture, TextureSubresourceSet subresources, ResourceStates stateBits) = 0;
virtual void beginTrackingBufferState(IBuffer* buffer, ResourceStates stateBits) = 0;
virtual void setTextureState(ITexture* texture, TextureSubresourceSet subresources, ResourceStates stateBits) = 0;
virtual void setBufferState(IBuffer* buffer, ResourceStates stateBits) = 0;
virtual void setAccelStructState(rt::IAccelStruct* as, ResourceStates stateBits) = 0;
virtual void setPermanentTextureState(ITexture* texture, ResourceStates stateBits) = 0;
virtual void setPermanentBufferState(IBuffer* buffer, ResourceStates stateBits) = 0;
virtual void commitBarriers() = 0;
virtual ResourceStates getTextureSubresourceState(ITexture* texture, ArraySlice arraySlice, MipLevel mipLevel) = 0;
virtual ResourceStates getBufferState(IBuffer* buffer) = 0;
```

**Debug Markers:**
```cpp
virtual void beginMarker(const char* name) = 0;
virtual void endMarker() = 0;
```

**Device Access:**
```cpp
virtual IDevice* getDevice() = 0;
virtual const CommandListParameters& getDesc() = 0;
```

#### Usage Example

```cpp
nvrhi::CommandListHandle cmdList = device->createCommandList();
cmdList->open();

// Set render state and clear
nvrhi::GraphicsState state;
state.pipeline = graphicsPipeline;
state.framebuffer = framebuffer;
state.bindingSets = { bindingSet };
state.viewport = viewport;
state.rasterState = rasterState;
state.blendState = blendState;
state.depthStencilState = depthStencilState;

cmdList->setGraphicsState(state);

// Clear render target
cmdList->clearTextureFloat(
    framebuffer->getDesc().colorAttachments[0].texture, 
    nvrhi::AllSubresources, 
    nvrhi::Color(0.1f, 0.1f, 0.1f, 1.0f)
);

// Issue draw call
nvrhi::DrawArguments args;
args.vertexCount = 3;
args.startVertex = 0;
cmdList->draw(args);

cmdList->close();
device->executeCommandList(cmdList);
```

### IResource

Base interface for all reference-counted resources.

**Header**: `include/nvrhi/common/resource.h`

#### Key Methods

```cpp
virtual void AddRef() = 0;
virtual void Release() = 0;
virtual uint32_t GetRefCount() const = 0;

// Native object access
template<typename T>
T* getNativeObject() const;
```

#### Usage Example

```cpp
// All NVRHI resources inherit from IResource
TextureHandle texture = device->createTexture(desc);
BufferHandle buffer = device->createBuffer(desc);

// Get native object
VkImage vulkanImage = texture->getNativeObject<VkImage>();
ID3D12Resource* d3dResource = buffer->getNativeObject<ID3D12Resource*>();
```

## Resource Types

### ITexture

Interface for texture resources, including 1D, 2D, 3D textures, arrays, and multisample textures.

**Header**: `include/nvrhi/nvrhi.h` (lines 541-549, 624-630)

#### Key Methods

```cpp
[[nodiscard]] virtual const TextureDesc& getDesc() const = 0;
virtual Object getNativeView(ObjectType objectType, Format format = Format::UNKNOWN, 
    TextureSubresourceSet subresources = AllSubresources, 
    TextureDimension dimension = TextureDimension::Unknown, 
    bool isReadOnlyDSV = false) = 0;
```

#### TextureDesc Structure

**Header**: `include/nvrhi/nvrhi.h` (lines 405-469)

```cpp
struct TextureDesc
{
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    uint32_t arraySize = 1;
    uint32_t mipLevels = 1;
    uint32_t sampleCount = 1;
    uint32_t sampleQuality = 0;
    Format format = Format::UNKNOWN;
    TextureDimension dimension = TextureDimension::Texture2D;
    std::string debugName;
    
    bool isShaderResource = true;  // Note: default is true for backward compatibility
    bool isRenderTarget = false;
    bool isUAV = false;
    bool isTypeless = false;
    bool isShadingRateSurface = false;
    
    SharedResourceFlags sharedResourceFlags = SharedResourceFlags::None;
    
    bool isVirtual = false;  // For virtual resources with memory bound later
    bool isTiled = false;
    
    Color clearValue;
    bool useClearValue = false;
    
    ResourceStates initialState = ResourceStates::Unknown;
    bool keepInitialState = false;  // Enable automatic state tracking
    
    // Chainable setters
    constexpr TextureDesc& setWidth(uint32_t value);
    constexpr TextureDesc& setHeight(uint32_t value);
    constexpr TextureDesc& setDepth(uint32_t value);
    constexpr TextureDesc& setArraySize(uint32_t value);
    constexpr TextureDesc& setMipLevels(uint32_t value);
    constexpr TextureDesc& setSampleCount(uint32_t value);
    constexpr TextureDesc& setSampleQuality(uint32_t value);
    constexpr TextureDesc& setFormat(Format value);
    constexpr TextureDesc& setDimension(TextureDimension value);
    TextureDesc& setDebugName(const std::string& value);
    constexpr TextureDesc& setIsRenderTarget(bool value);
    constexpr TextureDesc& setIsUAV(bool value);
    constexpr TextureDesc& setIsTypeless(bool value);
    constexpr TextureDesc& setIsVirtual(bool value);
    constexpr TextureDesc& setClearValue(const Color& value);
    constexpr TextureDesc& setUseClearValue(bool value);
    constexpr TextureDesc& setInitialState(ResourceStates value);
    constexpr TextureDesc& setKeepInitialState(bool value);
    constexpr TextureDesc& setSharedResourceFlags(SharedResourceFlags value);
    
    // Enable automatic state tracking (equivalent to setInitialState + setKeepInitialState)
    constexpr TextureDesc& enableAutomaticStateTracking(ResourceStates _initialState);
};
```

#### Usage Example

```cpp
// Create a 2D render target texture
nvrhi::TextureDesc rtDesc;
rtDesc.setWidth(1024)
    .setHeight(1024)
    .setFormat(nvrhi::Format::RGBA16_FLOAT)
    .setIsRenderTarget(true)
    .enableAutomaticStateTracking(nvrhi::ResourceStates::RenderTarget)
    .setClearValue(nvrhi::Color(0.0f, 0.0f, 0.0f, 1.0f))
    .setDebugName("MainRenderTarget");

nvrhi::TextureHandle renderTarget = device->createTexture(rtDesc);

// Create a texture array
nvrhi::TextureDesc arrayDesc;
arrayDesc.setWidth(512)
    .setHeight(512)
    .setArraySize(6)  // Cube map
    .setDimension(nvrhi::TextureDimension::TextureCube)
    .setFormat(nvrhi::Format::RGBA8_UNORM)
    .setIsShaderResource(true);

nvrhi::TextureHandle cubeMap = device->createTexture(arrayDesc);
```

### IBuffer

Interface for buffer resources of various types (vertex, index, constant, UAV, etc.)

**Header**: `include/nvrhi/nvrhi.h` (lines 756-763)

#### BufferDesc Structure

**Header**: `include/nvrhi/nvrhi.h` (lines 669-732)

```cpp
struct BufferDesc
{
    uint64_t byteSize = 0;
    uint32_t structStride = 0;  // For structured buffers
    uint32_t maxVersions = 0;   // For volatile buffers (Vulkan)
    std::string debugName;
    Format format = Format::UNKNOWN;  // For typed buffer views
    
    bool canHaveUAVs = false;
    bool canHaveTypedViews = false;
    bool canHaveRawViews = false;
    bool isVertexBuffer = false;
    bool isIndexBuffer = false;
    bool isConstantBuffer = false;
    bool isDrawIndirectArgs = false;
    bool isAccelStructBuildInput = false;
    bool isAccelStructStorage = false;
    bool isShaderBindingTable = false;
    
    bool isVolatile = false;  // For frequent updates (per-draw constants)
    bool isVirtual = false;   // For virtual resources with memory bound later
    
    ResourceStates initialState = ResourceStates::Common;
    bool keepInitialState = false;
    
    CpuAccessMode cpuAccess = CpuAccessMode::None;
    SharedResourceFlags sharedResourceFlags = SharedResourceFlags::None;
    
    // Chainable setters
    constexpr BufferDesc& setByteSize(uint64_t value);
    constexpr BufferDesc& setStructStride(uint32_t value);
    constexpr BufferDesc& setMaxVersions(uint32_t value);
    BufferDesc& setDebugName(const std::string& value);
    constexpr BufferDesc& setFormat(Format value);
    constexpr BufferDesc& setCanHaveUAVs(bool value);
    constexpr BufferDesc& setCanHaveTypedViews(bool value);
    constexpr BufferDesc& setCanHaveRawViews(bool value);
    constexpr BufferDesc& setIsVertexBuffer(bool value);
    constexpr BufferDesc& setIsIndexBuffer(bool value);
    constexpr BufferDesc& setIsConstantBuffer(bool value);
    constexpr BufferDesc& setIsDrawIndirectArgs(bool value);
    constexpr BufferDesc& setIsAccelStructBuildInput(bool value);
    constexpr BufferDesc& setIsAccelStructStorage(bool value);
    constexpr BufferDesc& setIsShaderBindingTable(bool value);
    constexpr BufferDesc& setIsVolatile(bool value);
    constexpr BufferDesc& setIsVirtual(bool value);
    constexpr BufferDesc& setInitialState(ResourceStates value);
    constexpr BufferDesc& setKeepInitialState(bool value);
    constexpr BufferDesc& setCpuAccess(CpuAccessMode value);
    
    // Enable automatic state tracking
    constexpr BufferDesc& enableAutomaticStateTracking(ResourceStates _initialState);
};
```

#### BufferRange Structure

**Header**: `include/nvrhi/nvrhi.h` (lines 734-752)

```cpp
struct BufferRange
{
    uint64_t byteOffset = 0;
    uint64_t byteSize = 0;
    
    [[nodiscard]] BufferRange resolve(const BufferDesc& desc) const;
    [[nodiscard]] constexpr bool isEntireBuffer(const BufferDesc& desc) const;
    
    constexpr BufferRange& setByteOffset(uint64_t value);
    constexpr BufferRange& setByteSize(uint64_t value);
};

static const BufferRange EntireBuffer = BufferRange(0, ~0ull);
```

#### Usage Example

```cpp
// Create a vertex buffer
nvrhi::BufferDesc vbDesc;
vbDesc.setByteSize(vertexDataSize)
    .setStructStride(sizeof(Vertex))
    .setIsVertexBuffer(true);

nvrhi::BufferHandle vertexBuffer = device->createBuffer(vbDesc);

// Upload data (in command list)
cmdList->writeBuffer(vertexBuffer, vertexData, vertexDataSize);

// Create a volatile constant buffer (efficient for per-draw updates)
nvrhi::BufferDesc vcbDesc;
vcbDesc.setByteSize(sizeof(Constants))
    .setIsConstantBuffer(true)
    .setIsVolatile(true);

nvrhi::BufferHandle vcb = device->createBuffer(vcbDesc);
cmdList->writeBuffer(vcb, &constants, sizeof(constants));
```

### IHeap

Interface for memory heap objects used for explicit memory management and virtual resources.

**Header**: `include/nvrhi/nvrhi.h` (lines 310-316, 299-308)

#### HeapDesc Structure

```cpp
enum class HeapType : uint8_t
{
    DeviceLocal,  // GPU-local memory
    Upload,      // CPU-visible, GPU-optimal
    Readback     // CPU-visible for reading back GPU data
};

struct HeapDesc
{
    uint64_t capacity = 0;
    HeapType type;
    std::string debugName;
    
    constexpr HeapDesc& setCapacity(uint64_t value);
    constexpr HeapDesc& setType(HeapType value);
    HeapDesc& setDebugName(const std::string& value);
};
```

#### Usage Example

```cpp
nvrhi::HeapDesc heapDesc;
heapDesc.setCapacity(256 * 1024 * 1024)  // 256 MB
    .setType(nvrhi::HeapType::DeviceLocal)
    .setDebugName("MyDeviceHeap");

nvrhi::HeapHandle heap = device->createHeap(heapDesc);

// Create virtual texture with memory bound later
nvrhi::TextureDesc textureDesc;
textureDesc.setWidth(4096)
    .setHeight(4096)
    .setFormat(nvrhi::Format::RGBA8_UNORM)
    .setIsVirtual(true);  // No memory allocated yet

nvrhi::TextureHandle texture = device->createTexture(textureDesc);

// Bind memory
nvrhi::MemoryRequirements reqs = device->getTextureMemoryRequirements(texture);
device->bindTextureMemory(texture, heap, 0);  // Bind at offset 0
```

### ISampler

Interface for sampler resources used in texture sampling operations.

**Header**: `include/nvrhi/nvrhi.h` (SamplerDesc structure)

#### SamplerDesc Structure

```cpp
struct SamplerDesc
{
    // Configuration fields and chainable setters
    // (See full definition in nvrhi.h)
};
```

#### Usage Example

```cpp
nvrhi::SamplerDesc samplerDesc;
samplerDesc.setAddressU(nvrhi::AddressMode::Wrap)
    .setAddressV(nvrhi::AddressMode::Wrap)
    .setAddressW(nvrhi::AddressMode::Wrap)
    .setFilter(nvrhi::FilterMode::Anisotropic)
    .setMaxAnisotropy(16);

nvrhi::SamplerHandle sampler = device->createSampler(samplerDesc);
```

### IInputLayout

Interface for input layout objects that define vertex attribute formats and bindings.

**Header**: `include/nvrhi/nvrhi.h` (lines 656-663, 636-654)

#### VertexAttributeDesc Structure

```cpp
struct VertexAttributeDesc
{
    std::string name;
    Format format = Format::UNKNOWN;
    uint32_t arraySize = 1;
    uint32_t bufferIndex = 0;
    uint32_t offset = 0;
    uint32_t elementStride = 0;
    bool isInstanced = false;
    
    VertexAttributeDesc& setName(const std::string& value);
    constexpr VertexAttributeDesc& setFormat(Format value);
    constexpr VertexAttributeDesc& setArraySize(uint32_t value);
    constexpr VertexAttributeDesc& setBufferIndex(uint32_t value);
    constexpr VertexAttributeDesc& setOffset(uint32_t value);
    constexpr VertexAttributeDesc& setElementStride(uint32_t value);
    constexpr VertexAttributeDesc& setIsInstanced(bool value);
};
```

#### Usage Example

```cpp
nvrhi::VertexAttributeDesc attributes[] = {
    nvrhi::VertexAttributeDesc()
        .setName("POSITION")
        .setFormat(nvrhi::Format::RGB32_FLOAT)
        .setOffset(0)
        .setBufferIndex(0)
        .setElementStride(sizeof(Vertex)),
    nvrhi::VertexAttributeDesc()
        .setName("TEXCOORD")
        .setFormat(nvrhi::Format::RG32_FLOAT)
        .setOffset(12)
        .setBufferIndex(0)
        .setElementStride(sizeof(Vertex))
};

nvrhi::InputLayoutHandle inputLayout = device->createInputLayout(
    attributes, 2, vertexShader);
```

### IFramebuffer

Interface for framebuffer objects that define render target and depth-stencil attachments.

**Header**: `include/nvrhi/nvrhi.h`

#### FramebufferDesc Structure

```cpp
struct FramebufferDesc
{
    // Render target and depth-stencil attachment definitions
};

struct FramebufferInfo
{
    static_vector<Format, c_MaxRenderTargets> colorFormats;
    Format depthFormat;
    uint32_t sampleCount;
    uint32_t sampleQuality;
};
```

#### Usage Example

```cpp
nvrhi::FramebufferDesc fbDesc;
fbDesc.colorAttachments[0].texture = renderTarget;
fbDesc.colorAttachments[0].subresources = nvrhi::AllSubresources;
fbDesc.depthAttachment.texture = depthStencil;
fbDesc.depthAttachment.subresources = nvrhi::AllSubresources;

nvrhi::FramebufferHandle framebuffer = device->createFramebuffer(fbDesc);
```

## Graphics Pipeline

### Graphics State

Encapsulates all state required for graphics rendering operations.

**Header**: `include/nvrhi/nvrhi.h`

```cpp
struct GraphicsState
{
    GraphicsPipelineHandle pipeline;
    static_vector<BindingSetHandle, c_MaxBindingLayouts> bindingSets;
    FramebufferHandle framebuffer;
    ViewportState viewport;
    DrawArguments drawArguments;  // For indirect draw parameters
    BlendState blendState;
    DepthStencilState depthStencilState;
    RasterState rasterState;
    VariableRateShadingState variableRateShadingState;
    
    // Setters for configuration
};
```

### Blend State

Controls how rendered colors are blended with existing framebuffer contents.

**Header**: `include/nvrhi/nvrhi.h` (lines 962-1024)

```cpp
enum class BlendFactor : uint8_t
{
    Zero = 1,
    One = 2,
    SrcColor = 3,
    InvSrcColor = 4,
    SrcAlpha = 5,
    InvSrcAlpha = 6,
    DstAlpha = 7,
    InvDstAlpha = 8,
    DstColor = 9,
    InvDstColor = 10,
    SrcAlphaSaturate = 11,
    ConstantColor = 14,
    InvConstantColor = 15,
    Src1Color = 16,
    InvSrc1Color = 17,
    Src1Alpha = 18,
    InvSrc1Alpha = 19
};

enum class BlendOp : uint8_t
{
    Add = 1,
    Subtract = 2,
    ReverseSubtract = 3,
    Min = 4,
    Max = 5
};

enum class ColorMask : uint8_t
{
    Red = 1,
    Green = 2,
    Blue = 4,
    Alpha = 8,
    All = 0xF
};

struct BlendState
{
    struct RenderTarget
    {
        bool blendEnable = false;
        BlendFactor srcBlend = BlendFactor::One;
        BlendFactor destBlend = BlendFactor::Zero;
        BlendOp blendOp = BlendOp::Add;
        BlendFactor srcBlendAlpha = BlendFactor::One;
        BlendFactor destBlendAlpha = BlendFactor::Zero;
        BlendOp blendOpAlpha = BlendOp::Add;
        ColorMask colorWriteMask = ColorMask::All;
        
        constexpr RenderTarget& setBlendEnable(bool enable);
        constexpr RenderTarget& enableBlend();
        constexpr RenderTarget& disableBlend();
        constexpr RenderTarget& setSrcBlend(BlendFactor value);
        constexpr RenderTarget& setDestBlend(BlendFactor value);
        constexpr RenderTarget& setBlendOp(BlendOp value);
        constexpr RenderTarget& setSrcBlendAlpha(BlendFactor value);
        constexpr RenderTarget& setDestBlendAlpha(BlendFactor value);
        constexpr RenderTarget& setBlendOpAlpha(BlendOp value);
        constexpr RenderTarget& setColorWriteMask(ColorMask value);
        
        [[nodiscard]] bool usesConstantColor() const;
    };
    
    RenderTarget targets[c_MaxRenderTargets];
    bool alphaToCoverageEnable = false;
};
```

### Raster State

Controls rasterization of primitives during rendering.

**Header**: `include/nvrhi/nvrhi.h`

```cpp
enum class RasterFillMode : uint8_t
{
    Solid,
    Wireframe
};

enum class RasterCullMode : uint8_t
{
    None,
    Front,
    Back
};

struct RasterState
{
    RasterFillMode fillMode = RasterFillMode::Solid;
    RasterCullMode cullMode = RasterCullMode::Back;
    bool frontCounterClockwise = false;
    bool depthClipEnable = false;
    bool scissorEnable = false;
    bool multisampleEnable = false;
    int depthBias = 0;
    float depthBiasClamp = 0.f;
    float slopeScaledDepthBias = 0.f;
    
    // Advanced features
    uint8_t forcedSampleCount = 0;
    bool programmableSamplePositionsEnable = false;
    bool conservativeRasterEnable = false;
    bool quadFillEnable = false;
};
```

### Depth Stencil State

Controls depth and stencil testing during rendering.

**Header**: `include/nvrhi/nvrhi.h`

```cpp
struct DepthStencilState
{
    // Depth and stencil configuration
    // (See full definition in nvrhi.h)
};
```

### Viewport State

Defines viewport and scissor rectangles for rendering.

**Header**: `include/nvrhi/nvrhi.h`

```cpp
struct ViewportState
{
    static_vector<Viewport, c_MaxViewports> viewports;
    static_vector<Rect, c_MaxViewports> scissors;
    
    // Setters for configuration
};

struct Viewport
{
    float minX, maxX;
    float minY, maxY;
    float minZ, maxZ;
    
    Viewport() : minX(0.f), maxX(0.f), minY(0.f), maxY(0.f), minZ(0.f), maxZ(1.f) { }
    Viewport(float width, float height);
    Viewport(float _minX, float _maxX, float _minY, float _maxY, float _minZ, float _maxZ);
    
    [[nodiscard]] float width() const { return maxX - minX; }
    [[nodiscard]] float height() const { return maxY - minY; }
};

struct Rect
{
    int minX, maxX;
    int minY, maxY;
    
    Rect() : minX(0), maxX(0), minY(0), maxY(0) { }
    Rect(int width, int height);
    Rect(int _minX, int _maxX, int _minY, int _maxY);
    explicit Rect(const Viewport& viewport);
    
    [[nodiscard]] int width() const { return maxX - minX; }
    [[nodiscard]] int height() const { return maxY - minY; }
};
```

## Shader and Pipeline

### Shader Types

**Header**: `include/nvrhi/nvrhi.h` (lines 770-794, 824-848)

```cpp
enum class ShaderType : uint16_t
{
    None            = 0x0000,
    
    Compute         = 0x0020,
    
    Vertex          = 0x0001,
    Hull            = 0x0002,
    Domain          = 0x0004,
    Geometry        = 0x0008,
    Pixel           = 0x0010,
    Amplification   = 0x0040,
    Mesh            = 0x0080,
    AllGraphics     = 0x00DF,
    
    RayGeneration   = 0x0100,
    AnyHit          = 0x0200,
    ClosestHit      = 0x0400,
    Miss            = 0x0800,
    Intersection    = 0x1000,
    Callable        = 0x2000,
    AllRayTracing   = 0x3F00,
    
    All             = 0x3FFF
};

NVRHI_ENUM_CLASS_FLAG_OPERATORS(ShaderType)

struct ShaderDesc
{
    ShaderType shaderType = ShaderType::None;
    std::string debugName;
    std::string entryName = "main";
    
    int hlslExtensionsUAV = -1;
    
    bool useSpecificShaderExt = false;
    uint32_t numCustomSemantics = 0;
    CustomSemantic* pCustomSemantics = nullptr;
    
    FastGeometryShaderFlags fastGSFlags = FastGeometryShaderFlags(0);
    uint32_t* pCoordinateSwizzling = nullptr;
    
    constexpr ShaderDesc& setShaderType(ShaderType value);
    ShaderDesc& setDebugName(const std::string& value);
    ShaderDesc& setEntryName(const std::string& value);
    constexpr ShaderDesc& setHlslExtensionsUAV(int value);
    constexpr ShaderDesc& setUseSpecificShaderExt(bool value);
    constexpr ShaderDesc& setCustomSemantics(uint32_t count, CustomSemantic* data);
    constexpr ShaderDesc& setFastGSFlags(FastGeometryShaderFlags value);
    constexpr ShaderDesc& setCoordinateSwizzling(uint32_t* value);
};
```

### Graphics Pipeline

**Header**: `include/nvrhi/nvrhi.h`

```cpp
struct GraphicsPipelineDesc
{
    // Pipeline configuration
    // (See full definition in nvrhi.h)
};

// Creation
GraphicsPipelineHandle pipeline = device->createGraphicsPipeline(pipelineDesc, framebufferInfo);
```

### Compute Pipeline

**Header**: `include/nvrhi/nvrhi.h`

```cpp
struct ComputePipelineDesc
{
    // Pipeline configuration
    // (See full definition in nvrhi.h)
};

// Creation
ComputePipelineHandle pipeline = device->createComputePipeline(pipelineDesc);
```

### Meshlet Pipeline

**Header**: `include/nvrhi/nvrhi.h`

```cpp
struct MeshletPipelineDesc
{
    // Pipeline configuration
    // (See full definition in nvrhi.h)
};

// Creation (not supported on DX11)
MeshletPipelineHandle pipeline = device->createMeshletPipeline(pipelineDesc, framebufferInfo);
```

## Ray Tracing

### Acceleration Structures

**Header**: `include/nvrhi/nvrhi.h` (Ray tracing interfaces)

```cpp
namespace rt {
    struct AccelStructDesc
    {
        bool bottomLevel = false;
        std::string debugName;
        // ... other fields
    };
    
    struct GeometryDesc
    {
        // Geometry definition for BLAS
        // (See full definition in nvrhi.h)
    };
    
    struct InstanceDesc
    {
        // Instance definition for TLAS
        // (See full definition in nvrhi.h)
    };
    
    enum class AccelStructBuildFlags : uint32_t
    {
        None = 0,
        AllowUpdate = 1,
        AllowCompaction = 2,
        // ... other flags
    };
    
    class IAccelStruct : public IResource
    {
    public:
        [[nodiscard]] virtual const AccelStructDesc& getDesc() const = 0;
    };
    
    typedef RefCountPtr<IAccelStruct> AccelStructHandle;
}
```

#### Building Acceleration Structures

```cpp
// Create BLAS
rt::AccelStructHandle blas = device->createAccelStruct(
    rt::AccelStructDesc()
        .setBottomLevel(true)
        .setDebugName("SceneBLAS")
);

// Create TLAS
rt::AccelStructHandle tlas = device->createAccelStruct(
    rt::AccelStructDesc()
        .setBottomLevel(false)
        .setDebugName("SceneTLAS")
);

// Build in command list
CommandListHandle buildCmdList = device->createCommandList();
buildCmdList->open();

// Build BLAS
rt::GeometryDesc geometry;
// ... configure geometry ...
buildCmdList->buildBottomLevelAccelStruct(blas, &geometry, 1, 
    rt::AccelStructBuildFlags::AllowUpdate | rt::AccelStructBuildFlags::AllowCompaction);

// Build TLAS
rt::InstanceDesc instance;
// ... configure instance ...
buildCmdList->buildTopLevelAccelStruct(tlas, &instance, 1,
    rt::AccelStructBuildFlags::AllowUpdate);

buildCmdList->close();
device->executeCommandList(buildCmdList);

// Compact if allowed
buildCmdList->open();
buildCmdList->compactBottomLevelAccelStructs();
buildCmdList->close();
device->executeCommandList(buildCmdList);
```

### Ray Tracing Pipeline

**Header**: `include/nvrhi/nvrhi.h`

```cpp
namespace rt {
    struct PipelineDesc
    {
        // Ray tracing pipeline configuration
        // (See full definition in nvrhi.h)
    };
    
    struct State
    {
        ShaderTableHandle shaderTable;
        static_vector<BindingSetHandle, c_MaxBindingLayouts> bindings;
        // ... other fields
    };
    
    struct DispatchRaysArguments
    {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t depth = 1;
        // ... other fields
    };
    
    class IPipeline : public IResource
    {
    public:
        [[nodiscard]] virtual const PipelineDesc& getDesc() const = 0;
    };
    
    typedef RefCountPtr<IPipeline> PipelineHandle;
}
```

#### Ray Tracing Dispatch

```cpp
// Create ray tracing pipeline
rt::PipelineHandle rtPipeline = device->createRayTracingPipeline(rtPipelineDesc);

// Create shader table
rt::ShaderTableHandle shaderTable = device->createShaderTable(rtShaderTableDesc);

// Set up ray tracing state
rt::State rtState;
rtState.shaderTable = shaderTable;
rtState.bindings = { bindingSet };

// Dispatch rays
CommandListHandle rtCmdList = device->createCommandList();
rtCmdList->open();

rtCmdList->setRayTracingState(rtState);

rt::DispatchRaysArguments args;
args.width = width;
args.height = height;
args.depth = 1;
rtCmdList->dispatchRays(args);

rtCmdList->close();
device->executeCommandList(rtCmdList);
```

### Opacity Micromap

**Header**: `include/nvrhi/nvrhi.h`

```cpp
namespace rt {
    struct OpacityMicromapDesc
    {
        std::string debugName;
        OpacityMicromapBuildFlags flags;
        std::vector<OpacityMicromapUsageCount> counts;
        IBuffer* inputBuffer = nullptr;
        uint64_t inputBufferOffset = 0;
        IBuffer* perOmmDescs = nullptr;
        uint64_t perOmmDescsOffset = 0;
    };
    
    class IOpacityMicromap : public IResource
    {
    public:
        [[nodiscard]] virtual const OpacityMicromapDesc& getDesc() const = 0;
    };
    
    typedef RefCountPtr<IOpacityMicromap> OpacityMicromapHandle;
}
```

#### Building Opacity Micromap

```cpp
// Create opacity micromap
rt::OpacityMicromapHandle omm = device->createOpacityMicromap(ommDesc);

// Build in command list
CommandListHandle buildCmdList = device->createCommandList();
buildCmdList->open();
buildCmdList->buildOpacityMicromap(omm, ommDesc);
buildCmdList->close();
device->executeCommandList(buildCmdList);
```

## Resource Binding

### Binding Layouts

**Header**: `include/nvrhi/nvrhi.h` (lines 2064-2071, 1993-2015)

```cpp
enum class ResourceType : uint8_t
{
    None,
    Texture_SRV,
    Texture_UAV,
    TypedBuffer_SRV,
    TypedBuffer_UAV,
    StructuredBuffer_SRV,
    StructuredBuffer_UAV,
    RawBuffer_SRV,
    RawBuffer_UAV,
    ConstantBuffer,
    VolatileConstantBuffer,
    Sampler,
    RayTracingAccelStruct,
    PushConstants,
    SamplerFeedbackTexture_UAV
};

struct BindingLayoutItem
{
    ResourceType type = ResourceType::None;
    uint32_t slot = 0;
    uint32_t size = 1;  // Array size, 0 for unbounded (bindless)
    uint32_t space = 0;
    TextureDimension dimension = TextureDimension::Unknown;
    Format format = Format::UNKNOWN;
    
    constexpr BindingLayoutItem& setType(ResourceType value);
    constexpr BindingLayoutItem& setSlot(uint32_t value);
    constexpr BindingLayoutItem& setSize(uint32_t value);
    constexpr BindingLayoutItem& setSpace(uint32_t value);
    constexpr BindingLayoutItem& setDimension(TextureDimension value);
    constexpr BindingLayoutItem& setFormat(Format value);
};

struct BindingLayoutDesc
{
    ShaderType visibility = ShaderType::None;
    uint32_t registerSpace = 0;
    bool registerSpaceIsDescriptorSet = false;  // Vulkan: map registerSpace to descriptor set
    std::vector<BindingLayoutItem> bindings;
    VulkanBindingOffsets bindingOffsets;
    
    BindingLayoutDesc& setVisibility(ShaderType value);
    BindingLayoutDesc& setRegisterSpace(uint32_t value);
    BindingLayoutDesc& setRegisterSpaceIsDescriptorSet(bool value);
    BindingLayoutDesc& setRegisterSpaceAndDescriptorSet(uint32_t value);
    BindingLayoutDesc& addItem(const BindingLayoutItem& value);
    BindingLayoutDesc& setBindingOffsets(const VulkanBindingOffsets& value);
};

class IBindingLayout : public IResource
{
public:
    [[nodiscard]] virtual const BindingLayoutDesc* getDesc() const = 0;
    [[nodiscard]] virtual const BindlessLayoutDesc* getBindlessDesc() const = 0;
};

typedef RefCountPtr<IBindingLayout> BindingLayoutHandle;
```

### Binding Sets

**Header**: `include/nvrhi/nvrhi.h` (lines 2392-2399, 2077-2354, 2360-2390)

```cpp
struct BindingSetItem
{
    IResource* resourceHandle;
    uint32_t slot;
    uint32_t arrayElement;
    ResourceType type : 8;
    TextureDimension dimension : 8;
    Format format : 8;
    uint8_t unused : 8;
    uint32_t unused2;
    union {
        TextureSubresourceSet subresources;  // For Texture_SRV, Texture_UAV
        BufferRange range;                   // For Buffer_SRV, Buffer_UAV, ConstantBuffer
        uint64_t rawData[2];
    };
    
    // Helper creation functions
    static BindingSetItem None(uint32_t slot = 0);
    static BindingSetItem Texture_SRV(uint32_t slot, ITexture* texture, Format format = Format::UNKNOWN,
        TextureSubresourceSet subresources = AllSubresources, TextureDimension dimension = TextureDimension::Unknown);
    static BindingSetItem Texture_UAV(uint32_t slot, ITexture* texture, Format format = Format::UNKNOWN,
        TextureSubresourceSet subresources = TextureSubresourceSet(0, 1, 0, TextureSubresourceSet::AllArraySlices),
        TextureDimension dimension = TextureDimension::Unknown);
    static BindingSetItem ConstantBuffer(uint32_t slot, IBuffer* buffer, BufferRange range = EntireBuffer);
    static BindingSetItem Sampler(uint32_t slot, ISampler* sampler);
    static BindingSetItem RayTracingAccelStruct(uint32_t slot, rt::IAccelStruct* as);
    // ... more helper functions
    
    BindingSetItem& setArrayElement(uint32_t value);
    BindingSetItem& setFormat(Format value);
    BindingSetItem& setDimension(TextureDimension value);
    BindingSetItem& setSubresources(TextureSubresourceSet value);
    BindingSetItem& setRange(BufferRange value);
};

struct BindingSetDesc
{
    std::vector<BindingSetItem> bindings;
    bool trackLiveness = true;  // Automatic liveness tracking
    
    BindingSetDesc& addItem(const BindingSetItem& value);
    BindingSetDesc& setTrackLiveness(bool value);
    
    bool operator ==(const BindingSetDesc& b) const;
    bool operator !=(const BindingSetDesc& b) const;
};

class IBindingSet : public IResource
{
public:
    [[nodiscard]] virtual const BindingSetDesc* getDesc() const = 0;
    [[nodiscard]] virtual IBindingLayout* getLayout() const = 0;
};

typedef RefCountPtr<IBindingSet> BindingSetHandle;
```

#### Usage Example

```cpp
// Create binding layout
BindingLayoutDesc layoutDesc;
layoutDesc.setVisibility(ShaderType::Pixel)
    .addItem(BindingLayoutItem()
        .setType(ResourceType::Texture_SRV)
        .setSlot(0)
        .setSpace(0))
    .addItem(BindingLayoutItem()
        .setType(ResourceType::Sampler)
        .setSlot(1)
        .setSpace(0));

BindingLayoutHandle layout = device->createBindingLayout(layoutDesc);

// Create binding set
BindingSetDesc setDesc;
setDesc.addItem(BindingSetItem::Texture_SRV(0, texture))
    .addItem(BindingSetItem::Sampler(1, sampler));

BindingSetHandle bindingSet = device->createBindingSet(setDesc, layout);

// Use in graphics state
GraphicsState state;
state.pipeline = pipeline;
state.bindingSets = { bindingSet };
state.framebuffer = framebuffer;
```

### Descriptor Tables

Mutable descriptor tables for dynamic binding updates.

**Header**: `include/nvrhi/nvrhi.h` (lines 2406-2413)

```cpp
class IDescriptorTable : public IBindingSet
{
public:
    [[nodiscard]] virtual uint32_t getCapacity() const = 0;
    [[nodiscard]] virtual uint32_t getFirstDescriptorIndexInHeap() const = 0;
};

typedef RefCountPtr<IDescriptorTable> DescriptorTableHandle;

// Usage
DescriptorTableHandle table = device->createDescriptorTable(layout);
device->resizeDescriptorTable(table, newCapacity, keepContents);
device->writeDescriptorTable(table, bindingItem);
```

## Specialized Features

### Variable Rate Shading

**Header**: `include/nvrhi/nvrhi.h`

```cpp
enum class VariableShadingRate : uint8_t
{
    e1x1,
    e1x2,
    e2x1,
    e2x2,
    e2x4,
    e4x2,
    e4x4
};

enum class ShadingRateCombiner : uint8_t
{
    Passthrough,
    Override,
    Min,
    Max,
    ApplyRelative
};

struct VariableRateShadingState
{
    bool enabled = false;
    VariableShadingRate shadingRate = VariableShadingRate::e1x1;
    ShadingRateCombiner pipelinePrimitiveCombiner = ShadingRateCombiner::Passthrough;
    ShadingRateCombiner imageCombiner = ShadingRateCombiner::Passthrough;
};

struct VariableRateShadingFeatureInfo
{
    uint32_t shadingRateImageTileSize;
};
```

### Sampler Feedback

**Header**: `include/nvrhi/nvrhi.h`

```cpp
enum SamplerFeedbackFormat : uint8_t
{
    MinMipOpaque = 0x0,
    MipRegionUsedOpaque = 0x1
};

struct SamplerFeedbackTextureDesc
{
    SamplerFeedbackFormat samplerFeedbackFormat = SamplerFeedbackFormat::MinMipOpaque;
    uint32_t samplerFeedbackMipRegionX = 0;
    uint32_t samplerFeedbackMipRegionY = 0;
    uint32_t samplerFeedbackMipRegionZ = 0;
    ResourceStates initialState = ResourceStates::Unknown;
    bool keepInitialState = false;
};

class ISamplerFeedbackTexture : public IResource
{
public:
    [[nodiscard]] virtual const SamplerFeedbackTextureDesc& getDesc() const = 0;
    virtual TextureHandle getPairedTexture() = 0;
};

typedef RefCountPtr<ISamplerFeedbackTexture> SamplerFeedbackTextureHandle;
```

### Cooperative Vectors

**Header**: `include/nvrhi/nvrhi.h`

```cpp
namespace coopvec {
    enum class DataType : uint8_t
    {
        // Data types for cooperative vector operations
    };
    
    enum class MatrixLayout : uint8_t
    {
        // Matrix layouts for cooperative vectors
    };
    
    struct DeviceFeatures
    {
        // Supported cooperative vector features
    };
    
    struct ConvertMatrixLayoutDesc
    {
        // Matrix conversion parameters
    };
}
```

## Query Objects

### Event Queries

GPU-CPU synchronization primitives.

**Header**: `include/nvrhi/nvrhi.h`

```cpp
class IEventQuery : public IResource
{
public:
    virtual void setCommandQueue(ICommandQueue* queue) = 0;
    virtual void wait() = 0;
};

typedef RefCountPtr<IEventQuery> EventQueryHandle;

// Usage
EventQueryHandle query = device->createEventQuery();
query->setCommandQueue(graphicsQueue);
device->executeCommandList(cmdList);
query->wait();  // Block until command list completes
```

### Timer Queries

GPU timing queries.

**Header**: `include/nvrhi/nvrhi.h`

```cpp
class ITimerQuery : public IResource
{
public:
    // Timer query interface
};

typedef RefCountPtr<ITimerQuery> TimerQueryHandle;

// Usage
TimerQueryHandle timerQuery = device->createTimerQuery();
cmdList->beginTimerQuery(timerQuery);
// ... rendering commands ...
cmdList->endTimerQuery(timerQuery);
device->executeCommandList(cmdList);

// Later, after query completes
if (device->pollTimerQuery(timerQuery)) {
    float gpuTimeSeconds = device->getTimerQueryTime(timerQuery);
}
```

## Utility Types

### Enums

#### GraphicsAPI

**Header**: `include/nvrhi/nvrhi.h` (lines 153-158)

```cpp
enum class GraphicsAPI : uint8_t
{
    D3D11,
    D3D12,
    VULKAN
};
```

#### ResourceStates

**Header**: `include/nvrhi/nvrhi.h` (lines 349-377)

```cpp
enum class ResourceStates : uint32_t
{
    Unknown                     = 0,
    Common                      = 0x00000001,
    ConstantBuffer              = 0x00000002,
    VertexBuffer                = 0x00000004,
    IndexBuffer                 = 0x00000008,
    IndirectArgument            = 0x00000010,
    ShaderResource              = 0x00000020,
    UnorderedAccess             = 0x00000040,
    RenderTarget                = 0x00000080,
    DepthWrite                  = 0x00000100,
    DepthRead                   = 0x00000200,
    StreamOut                   = 0x00000400,
    CopyDest                    = 0x00000800,
    CopySource                  = 0x00001000,
    ResolveDest                 = 0x00002000,
    ResolveSource               = 0x00004000,
    Present                     = 0x00008000,
    AccelStructRead             = 0x00010000,
    AccelStructWrite            = 0x00020000,
    AccelStructBuildInput       = 0x00040000,
    AccelStructBuildBlas        = 0x00080000,
    ShadingRateSurface          = 0x00100000,
    OpacityMicromapWrite        = 0x00200000,
    OpacityMicromapBuildInput   = 0x00400000,
    ConvertCoopVecMatrixInput   = 0x00800000,
    ConvertCoopVecMatrixOutput  = 0x01000000
};

NVRHI_ENUM_CLASS_FLAG_OPERATORS(ResourceStates)
```

#### Format

**Header**: `include/nvrhi/nvrhi.h` (lines 160-237)

```cpp
enum class Format : uint8_t
{
    UNKNOWN,
    
    // 8-bit formats
    R8_UINT, R8_SINT, R8_UNORM, R8_SNORM,
    RG8_UINT, RG8_SINT, RG8_UNORM, RG8_SNORM,
    
    // 16-bit formats
    R16_UINT, R16_SINT, R16_UNORM, R16_SNORM, R16_FLOAT,
    RG16_UINT, RG16_SINT, RG16_UNORM, RG16_SNORM, RG16_FLOAT,
    
    // 32-bit formats
    R32_UINT, R32_SINT, R32_FLOAT,
    RG32_UINT, RG32_SINT, RG32_FLOAT,
    RGB32_UINT, RGB32_SINT, RGB32_FLOAT,
    RGBA32_UINT, RGBA32_SINT, RGBA32_FLOAT,
    
    // Packed formats
    RGBA8_UINT, RGBA8_SINT, RGBA8_UNORM, RGBA8_SNORM,
    BGRA8_UNORM, BGRX8_UNORM, SRGBA8_UNORM, SBGRA8_UNORM, SBGRX8_UNORM,
    R10G10B10A2_UNORM, R11G11B10_FLOAT,
    BGRA4_UNORM, B5G6R5_UNORM, B5G5R5A1_UNORM,
    
    // 16-bit per-component
    RGBA16_UINT, RGBA16_SINT, RGBA16_FLOAT, RGBA16_UNORM, RGBA16_SNORM,
    
    // Depth-stencil formats
    D16, D24S8, X24G8_UINT, D32, D32S8, X32G8_UINT,
    
    // BC compressed formats
    BC1_UNORM, BC1_UNORM_SRGB,
    BC2_UNORM, BC2_UNORM_SRGB,
    BC3_UNORM, BC3_UNORM_SRGB,
    BC4_UNORM, BC4_SNORM,
    BC5_UNORM, BC5_SNORM,
    BC6H_UFLOAT, BC6H_SFLOAT,
    BC7_UNORM, BC7_UNORM_SRGB,
    
    COUNT
};
```

#### Feature

**Header**: `include/nvrhi/nvrhi.h` (lines 3014-3041)

```cpp
enum class Feature : uint8_t
{
    ComputeQueue,
    ConservativeRasterization,
    ConstantBufferRanges,
    CopyQueue,
    DeferredCommandLists,
    FastGeometryShader,
    HeapDirectlyIndexed,
    HlslExtensionUAV,
    LinearSweptSpheres,
    Meshlets,
    RayQuery,
    RayTracingAccelStruct,
    RayTracingClusters,
    RayTracingOpacityMicromap,
    RayTracingPipeline,
    SamplerFeedback,
    ShaderExecutionReordering,
    ShaderSpecializations,
    SinglePassStereo,
    Spheres,
    VariableRateShading,
    VirtualResources,
    WaveLaneCountMinMax,
    CooperativeVectorInferencing,
    CooperativeVectorTraining
};
```

### Constants

**Header**: `include/nvrhi/nvrhi.h` (lines 73-81)

```cpp
static constexpr uint32_t c_HeaderVersion = 23;
static constexpr uint32_t c_MaxRenderTargets = 8;
static constexpr uint32_t c_MaxViewports = 16;
static constexpr uint32_t c_MaxVertexAttributes = 16;
static constexpr uint32_t c_MaxBindingLayouts = 8;
static constexpr uint32_t c_MaxBindlessRegisterSpaces = 16;
static constexpr uint32_t c_MaxVolatileConstantBuffersPerLayout = 6;
static constexpr uint32_t c_MaxVolatileConstantBuffers = 32;
static constexpr uint32_t c_MaxPushConstantSize = 128;  // D3D12: 256 bytes, Vulkan: 128 bytes guaranteed
static constexpr uint32_t c_ConstantBufferOffsetSizeAlignment = 256;
```

### Structs

#### Color

**Header**: `include/nvrhi/nvrhi.h` (lines 87-97)

```cpp
struct Color
{
    float r, g, b, a;
    
    Color() : r(0.f), g(0.f), b(0.f), a(0.f) { }
    Color(float c) : r(c), g(c), b(c), a(c) { }
    Color(float _r, float _g, float _b, float _a) : r(_r), g(_g), b(_b), a(_a) { }
    
    bool operator ==(const Color& _b) const;
    bool operator !=(const Color& _b) const;
};
```

#### DrawArguments

**Header**: `include/nvrhi/nvrhi.h`

```cpp
struct DrawArguments
{
    uint32_t vertexCount = 0;
    uint32_t instanceCount = 1;
    uint32_t startVertex = 0;
    uint32_t startInstance = 0;
    uint32_t firstIndex = 0;  // For indexed draws
    int32_t vertexOffset = 0;  // For indexed draws
};
```

## Backend-Specific Extensions

### Vulkan Extensions

**Header**: `include/nvrhi/vulkan.h`

```cpp
namespace nvrhi::vulkan {
    class IDevice : public nvrhi::IDevice
    {
    public:
        virtual VkSemaphore getQueueSemaphore(CommandQueue queue) = 0;
        virtual void queueWaitForSemaphore(CommandQueue waitQueue, VkSemaphore semaphore, uint64_t value) = 0;
        virtual void queueSignalSemaphore(CommandQueue executionQueue, VkSemaphore semaphore, uint64_t value) = 0;
        virtual uint64_t queueGetCompletedInstance(CommandQueue queue) = 0;
    };
    
    struct DeviceDesc
    {
        IMessageCallback* errorCB = nullptr;
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkQueue graphicsQueue;
        int graphicsQueueIndex = -1;
        VkQueue transferQueue;
        int transferQueueIndex = -1;
        VkQueue computeQueue;
        int computeQueueIndex = -1;
        VkAllocationCallbacks *allocationCallbacks = nullptr;
        const char **instanceExtensions = nullptr;
        size_t numInstanceExtensions = 0;
        const char **deviceExtensions = nullptr;
        size_t numDeviceExtensions = 0;
        uint32_t maxTimerQueries = 256;
        bool bufferDeviceAddressSupported = false;
        bool aftermathEnabled = false;
        bool logBufferLifetime = false;
        std::string vulkanLibraryName;
    };
    
    DeviceHandle createDevice(const DeviceDesc& desc);
    VkFormat convertFormat(nvrhi::Format format);
    const char* resultToString(VkResult result);
}
```

### DirectX 12 Extensions

**Header**: `include/nvrhi/d3d12.h`

```cpp
namespace nvrhi::d3d12 {
    class IRootSignature : public IResource
    {
    };
    
    typedef RefCountPtr<IRootSignature> RootSignatureHandle;
    
    class ICommandList : public nvrhi::ICommandList
    {
    public:
        virtual bool allocateUploadBuffer(size_t size, void** pCpuAddress, D3D12_GPU_VIRTUAL_ADDRESS* pGpuAddress) = 0;
        virtual bool commitDescriptorHeaps() = 0;
        virtual D3D12_GPU_VIRTUAL_ADDRESS getBufferGpuVA(IBuffer* buffer) = 0;
        virtual void updateGraphicsVolatileBuffers() = 0;
        virtual void updateComputeVolatileBuffers() = 0;
    };
    
    typedef uint32_t DescriptorIndex;
    
    class IDescriptorHeap
    {
    public:
        virtual DescriptorIndex allocateDescriptors(uint32_t count) = 0;
        virtual DescriptorIndex allocateDescriptor() = 0;
        virtual void releaseDescriptors(DescriptorIndex baseIndex, uint32_t count) = 0;
        virtual void releaseDescriptor(DescriptorIndex index) = 0;
        virtual D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandle(DescriptorIndex index) = 0;
        virtual D3D12_CPU_DESCRIPTOR_HANDLE getCpuHandleShaderVisible(DescriptorIndex index) = 0;
        virtual D3D12_GPU_DESCRIPTOR_HANDLE getGpuHandle(DescriptorIndex index) = 0;
        [[nodiscard]] virtual ID3D12DescriptorHeap* getHeap() const = 0;
        [[nodiscard]] virtual ID3D12DescriptorHeap* getShaderVisibleHeap() const = 0;
    };
    
    enum class DescriptorHeapType
    {
        RenderTargetView,
        DepthStencilView,
        ShaderResourceView,
        Sampler
    };
    
    class IDevice : public nvrhi::IDevice
    {
    public:
        virtual RootSignatureHandle buildRootSignature(const static_vector<BindingLayoutHandle, c_MaxBindingLayouts>& pipelineLayouts, bool allowInputLayout, bool isLocal, const D3D12_ROOT_PARAMETER1* pCustomParameters = nullptr, uint32_t numCustomParameters = 0) = 0;
        virtual GraphicsPipelineHandle createHandleForNativeGraphicsPipeline(IRootSignature* rootSignature, ID3D12PipelineState* pipelineState, const GraphicsPipelineDesc& desc, const FramebufferInfo& framebufferInfo) = 0;
        virtual MeshletPipelineHandle createHandleForNativeMeshletPipeline(IRootSignature* rootSignature, ID3D12PipelineState* pipelineState, const MeshletPipelineDesc& desc, const FramebufferInfo& framebufferInfo) = 0;
        [[nodiscard]] virtual IDescriptorHeap* getDescriptorHeap(DescriptorHeapType heapType) = 0;
    };
    
    struct DeviceDesc
    {
        IMessageCallback* errorCB = nullptr;
        ID3D12Device* pDevice = nullptr;
        ID3D12CommandQueue* pGraphicsCommandQueue = nullptr;
        ID3D12CommandQueue* pComputeCommandQueue = nullptr;
        ID3D12CommandQueue* pCopyCommandQueue = nullptr;
        uint32_t renderTargetViewHeapSize = 1024;
        uint32_t depthStencilViewHeapSize = 1024;
        uint32_t shaderResourceViewHeapSize = 16384;
        uint32_t samplerHeapSize = 1024;
        uint32_t maxTimerQueries = 256;
        bool enableHeapDirectlyIndexed = false;
        bool aftermathEnabled = false;
        bool logBufferLifetime = false;
    };
    
    DeviceHandle createDevice(const DeviceDesc& desc);
    DXGI_FORMAT convertFormat(nvrhi::Format format);
}
```

## Best Practices

### Resource State Management

1. **Use automatic state tracking** when possible by enabling `keepInitialState` in resource descriptors
2. **Set permanent states** for resources that don't change to reduce state tracking overhead
3. **Call `runGarbageCollection()` regularly** - at least once per frame - to clean up released resources
4. **Disable UAV barriers** for independent operations to improve performance

### Memory Management

1. **Use RefCountPtr** for automatic lifetime management
2. **Prefer volatile buffers** for frequently updated constant data (per-draw constants)
3. **Use staging textures** for CPU readback of GPU resources
4. **Bind memory explicitly** for virtual resources using `bindTextureMemory()` or `bindBufferMemory()`

### Performance Optimization

1. **Batch state changes** by using `setGraphicsState()` rather than individual setters
2. **Use multiple command lists** in parallel for different GPU queues
3. **Enable automatic UAV barriers** only when needed to avoid unnecessary synchronization
4. **Use descriptor tables** for mutable bindings that change frequently
5. **Use bindless layouts** for large, unbounded descriptor arrays
6. **Profile with GPU markers** using `beginMarker()`/`endMarker()` for debugging

### Cross-Platform Considerations

1. **Check feature support** before using advanced features with `queryFeatureSupport()`
2. **Be aware of API-specific limitations** (e.g., meshlets not supported on DX11)
3. **Use format compatibility checks** when creating resources
4. **Consider memory layout differences** between APIs (especially for push constants)
   - D3D12: 256-byte root parameter space
   - Vulkan: 128-byte push constant space guaranteed

### Debugging

1. **Enable validation layer** during development
2. **Use debug markers** with `beginMarker()`/`endMarker()` for GPU profiling
3. **Check return values** from all creation functions - they return null on failure
4. **Use Aftermath integration** for crash analysis (when available)
5. **Implement message callback** to receive errors and warnings

### Resource Creation Pattern

```cpp
// 1. Create descriptor
nvrhi::TextureDesc desc;
desc.setWidth(size)
    .setHeight(size)
    .setFormat(format)
    .setIsRenderTarget(true)
    .enableAutomaticStateTracking(nvrhi::ResourceStates::RenderTarget)
    .setDebugName("MyRenderTarget");

// 2. Create resource
nvrhi::TextureHandle texture = device->createTexture(desc);

// 3. Check for success
if (!texture)
{
    // Handle creation failure
    logError("Failed to create texture");
    return;
}
```

### Command Recording Pattern

```cpp
// 1. Create and open command list
nvrhi::CommandListHandle cmdList = device->createCommandList();
cmdList->open();

// 2. Set state
cmdList->setGraphicsState(state);

// 3. Issue commands
cmdList->drawIndexed(args);

// 4. Close and execute
cmdList->close();
device->executeCommandList(cmdList);

// 5. Clean up (periodically, at least once per frame)
device->runGarbageCollection();
```

This API reference guide provides comprehensive documentation for the NVRHI API. For more detailed implementation examples and advanced use cases, refer to the NVRHI samples and programming guide in the `doc/` directory.
