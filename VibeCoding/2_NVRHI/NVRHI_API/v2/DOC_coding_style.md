# NVRHI Coding Style Guide

## Table of Contents
1. [Overview](#overview)
2. [Naming Conventions](#naming-conventions)
3. [Code Organization](#code-organization)
4. [Graphics API Abstraction](#graphics-api-abstraction)
5. [Resource Management](#resource-management)
6. [Error Handling](#error-handling)
7. [Synchronization and State Tracking](#synchronization-and-state-tracking)
8. [Performance Considerations](#performance-considerations)
9. [Multi-API Support](#multi-api-support)
10. [Implementation Patterns](#implementation-patterns)

## Overview

NVRHI (NVIDIA Rendering Hardware Interface) is a cross-platform graphics API abstraction library that provides a unified interface for DirectX 11, DirectX 12, and Vulkan 1.3. It works on Windows (x64 only) and Linux (x64 and ARM64).

The codebase follows a modern C++17/23 style with careful attention to performance, memory management, and cross-platform compatibility. The design emphasizes abstraction efficiency while maintaining the ability to access native API resources when necessary.

Key design principles:
- **Interface-based architecture**: All resources are accessed through pure virtual interfaces
- **Reference counting**: COM-style reference counting for automatic lifetime management
- **Zero-overhead abstraction**: Minimal runtime overhead compared to native APIs
- **Explicit control**: Easy access to native API objects when needed
- **Automatic state management**: Optional automatic resource state tracking and barrier placement

## Naming Conventions

### General Naming

- **Classes/Interfaces**: PascalCase (e.g., `IDevice`, `ICommandList`, `Texture`)
- **Methods**: camelCase (e.g., `createTexture()`, `setGraphicsState()`, `executeCommandLists()`)
- **Variables**: camelCase (e.g., `textureDesc`, `commandQueue`, `m_device`)
- **Constants**: `c_` prefix + PascalCase (e.g., `c_MaxRenderTargets`, `c_HeaderVersion`, `c_MaxVertexAttributes`)
- **Enums**: PascalCase with enum values in PascalCase (e.g., `ResourceStates::ShaderResource`, `Format::RGBA8_UNORM`)
- **Namespaces**: lower_case (e.g., `nvrhi::vulkan`, `nvrhi::d3d12`)
- **Template parameters**: Upper case (e.g., `T`, `U`)

### Resource Handles

Resource handles use typedef names ending with "Handle":
```cpp
typedef RefCountPtr<ITexture> TextureHandle;
typedef RefCountPtr<IBuffer> BufferHandle;
typedef RefCountPtr<IPipeline> PipelineHandle;
typedef RefCountPtr<ICommandList> CommandListHandle;
typedef RefCountPtr<IDevice> DeviceHandle;
```

### API-Specific Implementations

Backend implementations are nested in the `nvrhi` namespace:
```cpp
namespace nvrhi {
    namespace vulkan { 
        DeviceHandle createDevice(const DeviceDesc& desc);
    }
    namespace d3d11 { 
        DeviceHandle createDevice(ID3D11Device* device);
    }
    namespace d3d12 { 
        DeviceHandle createDevice(const DeviceDesc& desc);
    }
}
```

### Member Variables

Private member variables use `m_` prefix:
```cpp
class Device {
private:
    vk::Device m_device;
    std::unique_ptr<CommandAllocator> m_commandAllocator;
    uint32_t m_graphicsQueueFamily;
    CommandQueue m_queueType;
};
```

### Constants

Global constants use `c_` prefix followed by PascalCase:
```cpp
static constexpr uint32_t c_HeaderVersion = 23;
static constexpr uint32_t c_MaxRenderTargets = 8;
static constexpr uint32_t c_MaxViewports = 16;
static constexpr uint32_t c_MaxVertexAttributes = 16;
static constexpr uint32_t c_MaxBindingLayouts = 8;
static constexpr uint32_t c_MaxPushConstantSize = 128;
static constexpr uint32_t c_ConstantBufferOffsetSizeAlignment = 256;
```

## Code Organization

### Directory Structure

```
NVRHI/
├── include/nvrhi/           # Public API headers
│   ├── nvrhi.h             # Main API header (3845 lines)
│   ├── nvrhiHLSL.h         # HLSL-specific utilities
│   ├── utils.h             # Utility functions
│   ├── validation.h        # Validation layer interface
│   ├── common/             # Common headers
│   │   ├── containers.h    # Custom containers (static_vector)
│   │   ├── resource.h      # Base resource interfaces
│   │   ├── aftermath.h     # Aftermath crash dump support
│   │   └── resourcebindingmap.h
│   ├── d3d11.h             # D3D11 backend interface
│   ├── d3d12.h             # D3D12 backend interface  
│   └── vulkan.h            # Vulkan backend interface
├── src/                    # Implementation
│   ├── common/             # Shared implementation code
│   │   ├── dxgi-format.*   # DXGI format conversion
│   │   ├── format-info.*   # Format information tables
│   │   ├── misc.*          # Miscellaneous utilities
│   │   ├── state-tracking.*# Resource state tracking
│   │   ├── utils.*         # Utility implementations
│   │   ├── versioning.h    # Version management
│   │   └── aftermath.*     # Aftermath integration
│   ├── d3d11/              # D3D11 backend implementation
│   ├── d3d12/              # D3D12 backend implementation
│   ├── vulkan/             # Vulkan backend implementation
│   └── validation/         # Validation layer implementation
├── thirdparty/             # Third-party dependencies
└── rtxmu/                  # RTXMU integration (optional)
```

### Header Structure

All headers follow the MIT license header and use `#pragma once`:
```cpp
/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <standard_library_headers>
#include <nvrhi/headers.h>

namespace nvrhi {
    // Code here
}
```

### Class Organization

Classes follow a consistent pattern:
1. Public interface methods first (pure virtual for interfaces)
2. Protected methods next (for base classes)
3. Private members last
4. Helper functions in anonymous namespace or private methods

Example:
```cpp
class IDevice : public IResource
{
public:
    // Resource creation
    virtual HeapHandle createHeap(const HeapDesc& d) = 0;
    virtual TextureHandle createTexture(const TextureDesc& d) = 0;
    virtual BufferHandle createBuffer(const BufferDesc& d) = 0;
    
    // Pipeline creation
    virtual GraphicsPipelineHandle createGraphicsPipeline(const GraphicsPipelineDesc& desc, FramebufferInfo const& fbinfo) = 0;
    virtual ComputePipelineHandle createComputePipeline(const ComputePipelineDesc& desc) = 0;
    
    // Command list management
    virtual CommandListHandle createCommandList(const CommandListParameters& params = CommandListParameters()) = 0;
    virtual uint64_t executeCommandLists(ICommandList* const* pCommandLists, size_t numCommandLists, CommandQueue executionQueue = CommandQueue::Graphics) = 0;
    
    // Feature support
    virtual bool queryFeatureSupport(Feature feature, void* pInfo = nullptr, size_t infoSize = 0) = 0;
    virtual FormatSupport queryFormatSupport(Format format) = 0;
    
protected:
    ~IDevice() = default;  // Protected destructor for interfaces
};
```

## Graphics API Abstraction

### Interface Design Pattern

NVRHI uses a common interface pattern with backend-specific implementations:
```cpp
// Common interface
class IDevice {
public:
    virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
    virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
    virtual CommandListHandle createCommandList(const CommandListParameters& params = CommandListParameters()) = 0;
    // ... other methods
};

// Backend-specific implementation
class VulkanDevice : public IDevice {
public:
    TextureHandle createTexture(const TextureDesc& desc) override;
    BufferHandle createBuffer(const BufferDesc& desc) override;
    CommandListHandle createCommandList(const CommandListParameters& params) override;
    // ... implementation
private:
    vk::Device m_device;
    vk::PhysicalDevice m_physicalDevice;
    // ... other members
};
```

### Conversion Functions

Each backend provides conversion functions between NVRHI types and native API types:
```cpp
namespace nvrhi::vulkan {
    VkFormat convertFormat(Format format);
    VkPrimitiveTopology convertPrimitiveTopology(PrimitiveType topology);
    // ... other conversions
}

namespace nvrhi::d3d12 {
    DXGI_FORMAT convertFormat(Format format);
    D3D12_PRIMITIVE_TOPOLOGY convertPrimitiveTopology(PrimitiveType topology);
    // ... other conversions
}
```

### Feature Detection

Capabilities are detected at runtime and exposed through the device interface:
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

// Usage
bool rtSupported = device->queryFeatureSupport(Feature::RayTracingPipeline);

struct WaveLaneCountMinMaxFeatureInfo
{
    uint32_t minWaveLaneCount;
    uint32_t maxWaveLaneCount;
};

WaveLaneCountMinMaxFeatureInfo waveInfo;
if (device->queryFeatureSupport(Feature::WaveLaneCountMinMax, &waveInfo, sizeof(waveInfo)))
{
    // Use wave lane count information
}
```

## Resource Management

### Reference Counting System

NVRHI uses COM-style reference counting for all resources:
```cpp
class IResource {
public:
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual uint32_t GetRefCount() const = 0;
};

template<typename T>
class RefCountPtr {
private:
    T* m_ptr;
public:
    RefCountPtr(T* ptr = nullptr) : m_ptr(ptr) { if (m_ptr) m_ptr->AddRef(); }
    RefCountPtr(const RefCountPtr& other) : m_ptr(other.m_ptr) { if (m_ptr) m_ptr->AddRef(); }
    ~RefCountPtr() { if (m_ptr) m_ptr->Release(); }
    
    T* Get() const { return m_ptr; }
    T* operator->() const { return m_ptr; }
    T& operator*() const { return *m_ptr; }
    
    RefCountPtr& operator=(const RefCountPtr& other) {
        if (other.m_ptr) other.m_ptr->AddRef();
        if (m_ptr) m_ptr->Release();
        m_ptr = other.m_ptr;
        return *this;
    }
};
```

### Lifetime Management

Resources are tracked by the device and safely destroyed when no longer used:
```cpp
class Device {
private:
    std::vector<std::unique_ptr<ResourceWrapper>> m_deferredDeletes;
    
public:
    void runGarbageCollection() {
        // Process deferred deletions
        for (auto& resource : m_deferredDeletes) {
            if (resource->safeToDelete()) {
                resource->destroy();
            }
        }
        // Remove safely deleted resources
        m_deferredDeletes.erase(
            std::remove_if(m_deferredDeletes.begin(), m_deferredDeletes.end(),
                [](const auto& r) { return r->safeToDelete(); }),
            m_deferredDeletes.end()
        );
    }
};

// IMPORTANT: Call runGarbageCollection() at least once per frame
device->runGarbageCollection();
```

### Native Object Access

Direct access to native API objects is provided when needed:
```cpp
class IResource {
public:
    template<typename T>
    T* getNativeObject() const {
        // Returns native API object (VkImage, ID3D12Resource, etc.)
    }
    
    Object getNativeView(ObjectType objectType, Format format = Format::UNKNOWN, 
        TextureSubresourceSet subresources = AllSubresources, 
        TextureDimension dimension = TextureDimension::Unknown, 
        bool isReadOnlyDSV = false);
};

// Usage example
VkImage vulkanImage = texture->getNativeObject<VkImage>();
ID3D12Resource* d3d12Resource = buffer->getNativeObject<ID3D12Resource*>();
```

### Object Types

Each backend defines object type constants for type-safe native object access:
```cpp
namespace nvrhi::ObjectTypes {
    constexpr ObjectType Nvrhi_VK_Device = 0x00030101;
    constexpr ObjectType Nvrhi_D3D12_Device = 0x00020101;
    constexpr ObjectType Nvrhi_D3D12_CommandList = 0x00020102;
}
```

## Error Handling

### Return Value Patterns

Functions return error codes or null pointers for failures:
```cpp
// Boolean return pattern
bool setGraphicsState(const GraphicsState& state);

// Null pointer pattern for creation functions
TextureHandle createTexture(const TextureDesc& desc); // Returns null on failure

// Error code pattern for complex operations (less common)
Result createSwapChain(const SwapChainDesc& desc, SwapChainHandle& handle);
```

### Message Callback

Applications can implement a message callback to receive errors and warnings:
```cpp
enum class MessageSeverity : uint8_t
{
    Info,
    Warning,
    Error,
    Fatal
};

class IMessageCallback
{
protected:
    IMessageCallback() = default;
    virtual ~IMessageCallback() = default;

public:
    virtual void message(MessageSeverity severity, const char* messageText) = 0;
    
    IMessageCallback(const IMessageCallback&) = delete;
    IMessageCallback(const IMessageCallback&&) = delete;
    IMessageCallback& operator=(const IMessageCallback&) = delete;
    IMessageCallback& operator=(const IMessageCallback&&) = delete;
};

// Usage in device creation
struct DeviceDesc {
    IMessageCallback* errorCB = nullptr;
    // ... other fields
};

class MyMessageCallback : public IMessageCallback {
public:
    void message(MessageSeverity severity, const char* messageText) override {
        if (severity == MessageSeverity::Error || severity == MessageSeverity::Fatal) {
            logError(messageText);
        } else if (severity == MessageSeverity::Warning) {
            logWarning(messageText);
        }
    }
};
```

### Assertions

Debug builds use assertions for invariant checking:
```cpp
#define CHECK_VK_RETURN(res) if ((res) != vk::Result::eSuccess) { return res; }
#define ASSERT_VK_OK(res) assert((res) == vk::Result::eSuccess)

// Validation layer provides runtime checking
class ValidationDevice : public DeviceWrapper {
public:
    TextureHandle createTexture(const TextureDesc& desc) override {
        validateTextureDesc(desc);
        return m_device->createTexture(desc);
    }
};
```

## Synchronization and State Tracking

### Resource State Tracking

NVRHI automatically tracks and manages resource states:
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
    ConvertCoopVecMatrixOutput  = 0x01000000,
};

class CommandList {
private:
    std::unordered_map<IResource*, ResourceStates> m_resourceStates;
    
public:
    void beginTrackingTextureState(ITexture* texture, ResourceStates state);
    void setTextureState(ITexture* texture, ResourceStates state);
    void setPermanentTextureState(ITexture* texture, ResourceStates state);
    ResourceStates getTextureSubresourceState(ITexture* texture, ArraySlice arraySlice, MipLevel mipLevel);
};
```

### Barrier Management

Automatic barrier placement reduces boilerplate:
```cpp
// Automatic barriers based on usage (default behavior)
CommandList->setGraphicsState(graphicsState); // Inserts barriers as needed

// Manual barrier control
CommandList->setEnableAutomaticBarriers(false);
CommandList->setTextureState(texture, ResourceStates::RenderTarget);
CommandList->commitBarriers();

// UAV barrier control for performance
CommandList->setEnableUavBarriersForTexture(texture, false); // Disable for independent operations
```

### Synchronization Primitives

Event queries for GPU-CPU sync:
```cpp
class IEventQuery {
public:
    virtual void setCommandQueue(ICommandQueue* queue) = 0;
    virtual void wait() = 0;
};

// Usage
EventQueryHandle query = device->createEventQuery();
query->setCommandQueue(graphicsQueue);
device->executeCommandList(cmdList);
query->wait(); // Block until command list completes
```

Timer queries for GPU timing:
```cpp
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

Multi-queue synchronization:
```cpp
device->queueWaitForCommandList(computeQueue, graphicsQueue, graphicsCommandListInstance);
```

## Performance Considerations

### Volatile Constant Buffers

Special handling for frequently updated constant buffers:
```cpp
// Efficient for per-draw updates
BufferDesc vcbDesc;
vcbDesc.setByteSize(sizeof(Constants))
    .setIsConstantBuffer(true)
    .setIsVolatile(true);  // Key flag for performance

BufferHandle vcb = device->createBuffer(vcbDesc);
commandList->writeBuffer(vcb, &constants, sizeof(constants));
// Updates are sub-allocated and tracked per command list
```

### Memory Allocation Patterns

Static vector for compile-time sized arrays:
```cpp
template <typename T, uint32_t _max_elements>
struct static_vector : private std::array<T, _max_elements> {
    // Stack-allocated with fixed capacity
    // Used extensively in NVRHI for performance-critical paths
};

// Example usage in GraphicsState
struct GraphicsState {
    GraphicsPipelineHandle pipeline;
    static_vector<BindingSetHandle, c_MaxBindingLayouts> bindingSets;
    FramebufferHandle framebuffer;
    ViewportState viewport;
    // ... other fields
};
```

Alignment helpers:
```cpp
template<typename T> 
T align(T size, T alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// Used for constant buffer offsets
static constexpr uint32_t c_ConstantBufferOffsetSizeAlignment = 256;
```

### Command Buffer Reuse

Command lists manage multiple native command buffers for efficiency:
```cpp
class CommandList {
private:
    std::vector<NativeCommandList> m_commandLists;
    uint32_t m_currentIndex = 0;
    
public:
    void close() {
        m_currentIndex = (m_currentIndex + 1) % m_commandLists.size();
        // Reuse command buffers from previous frames
    }
};
```

### Descriptor Management

Efficient descriptor allocation and binding:
```cpp
// Bindless layouts for large descriptor arrays
struct BindlessLayoutDesc {
    enum class LayoutType {
        Immutable = 0,
        MutableSrvUavCbv,    // Corresponds to ResourceDescriptorHeap
        MutableCounters,     // Counter resources
        MutableSampler       // SamplerDescriptorHeap
    };
    
    ShaderType visibility;
    uint32_t firstSlot;
    uint32_t maxCapacity;  // Unbounded array size
    LayoutType layoutType;
};

// Descriptor tables for mutable bindings
DescriptorTableHandle table = device->createDescriptorTable(layout);
device->resizeDescriptorTable(table, newCapacity, keepContents);
device->writeDescriptorTable(table, bindingItem);
```

## Multi-API Support

### Conditional Compilation

Features are conditionally compiled based on available APIs:
```cpp
#if defined(NVRHI_WITH_VULKAN)
    // Vulkan-specific code
    #include <vulkan/vulkan.h>
#elif defined(NVRHI_WITH_DX12)
    // D3D12-specific code
    #include <directx/d3d12.h>
#elif defined(NVRHI_WITH_DX11)
    // D3D11-specific code
    #include <d3d11.h>
#endif
```

### API-Specific Extensions

Each backend provides additional interfaces:
```cpp
// Vulkan-specific interface
namespace nvrhi::vulkan {
    class IDevice : public nvrhi::IDevice {
    public:
        virtual VkSemaphore getQueueSemaphore(CommandQueue queue) = 0;
        virtual void queueWaitForSemaphore(CommandQueue waitQueue, VkSemaphore semaphore, uint64_t value) = 0;
        virtual void queueSignalSemaphore(CommandQueue executionQueue, VkSemaphore semaphore, uint64_t value) = 0;
        virtual uint64_t queueGetCompletedInstance(CommandQueue queue) = 0;
    };
}

// D3D12-specific interface
namespace nvrhi::d3d12 {
    class IDevice : public nvrhi::IDevice {
    public:
        virtual RootSignatureHandle buildRootSignature(const static_vector<BindingLayoutHandle, c_MaxBindingLayouts>& pipelineLayouts, bool allowInputLayout, bool isLocal, const D3D12_ROOT_PARAMETER1* pCustomParameters = nullptr, uint32_t numCustomParameters = 0) = 0;
        virtual GraphicsPipelineHandle createHandleForNativeGraphicsPipeline(IRootSignature* rootSignature, ID3D12PipelineState* pipelineState, const GraphicsPipelineDesc& desc, const FramebufferInfo& framebufferInfo) = 0;
        [[nodiscard]] virtual IDescriptorHeap* getDescriptorHeap(DescriptorHeapType heapType) = 0;
    };
    
    class ICommandList : public nvrhi::ICommandList {
    public:
        virtual bool allocateUploadBuffer(size_t size, void** pCpuAddress, D3D12_GPU_VIRTUAL_ADDRESS* pGpuAddress) = 0;
        virtual bool commitDescriptorHeaps() = 0;
        virtual D3D12_GPU_VIRTUAL_ADDRESS getBufferGpuVA(IBuffer* buffer) = 0;
    };
}
```

### Feature Detection

Runtime feature detection enables graceful degradation:
```cpp
struct DeviceCapabilities {
    bool rayTracing = false;
    bool meshShading = false;
    bool variableRateShading = false;
    uint32_t maxRTGeometries = 0;
};

// Usage pattern
if (device->queryFeatureSupport(Feature::RayTracingPipeline)) {
    // Use ray tracing
} else {
    // Fallback path
}

if (device->queryFeatureSupport(Feature::Meshlets)) {
    // Use mesh shaders
} else {
    // Traditional geometry pipeline
}
```

### Platform-Specific Optimizations

```cpp
// D3D12: Root constants (256-byte root parameter space)
if (isD3D12) {
    // Use root constants for push constants
    cmdList->setPushConstants(&data, sizeof(data));
    // Maps to SetGraphicsRoot32BitConstants
}

// Vulkan: Push constants (128-byte guaranteed)
if (isVulkan) {
    // Use push constants
    cmdList->setPushConstants(&data, sizeof(data));
    // Maps to vkCmdPushConstants
}

// DX11: Shared buffer
if (isD3D11) {
    // Single buffer for all pipelines
    cmdList->setPushConstants(&data, sizeof(data));
    // Maps to UpdateSubresource
}
```

## Implementation Patterns

### Factory Pattern

Backend creation uses factory functions:
```cpp
// Vulkan device creation
namespace nvrhi::vulkan {
    struct DeviceDesc {
        IMessageCallback* errorCB = nullptr;
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        VkDevice device;
        VkQueue graphicsQueue;
        int graphicsQueueIndex = -1;
        // ... other fields
    };
    
    DeviceHandle createDevice(const DeviceDesc& desc);
}

// D3D12 device creation
namespace nvrhi::d3d12 {
    struct DeviceDesc {
        IMessageCallback* errorCB = nullptr;
        ID3D12Device* pDevice = nullptr;
        ID3D12CommandQueue* pGraphicsCommandQueue = nullptr;
        ID3D12CommandQueue* pComputeCommandQueue = nullptr;
        ID3D12CommandQueue* pCopyCommandQueue = nullptr;
        uint32_t renderTargetViewHeapSize = 1024;
        uint32_t depthStencilViewHeapSize = 1024;
        uint32_t shaderResourceViewHeapSize = 16384;
        uint32_t samplerHeapSize = 1024;
        bool enableHeapDirectlyIndexed = false;
        bool aftermathEnabled = false;
    };
    
    DeviceHandle createDevice(const DeviceDesc& desc);
}
```

### Wrapper Pattern

Native objects are wrapped rather than inherited:
```cpp
class VulkanTexture : public ITexture {
private:
    VkImage m_image;
    VkImageView m_imageView;
    TextureDesc m_desc;
    ResourceStates m_currentState;
    
public:
    const TextureDesc& getDesc() const override { return m_desc; }
    
    void* getNativeObject() override { 
        return &m_image; 
    }
    
    Object getNativeView(ObjectType objectType, Format format, 
        TextureSubresourceSet subresources, 
        TextureDimension dimension, bool isReadOnlyDSV) override {
        // Return appropriate view based on objectType
        if (objectType == ObjectTypes::Nvrhi_VK_ImageView) {
            return &m_imageView;
        }
        return nullptr;
    }
};
```

### Chainable Setter Pattern

Descriptors use chainable setters for fluent configuration:
```cpp
struct TextureDesc {
    uint32_t width = 1;
    uint32_t height = 1;
    Format format = Format::UNKNOWN;
    bool isRenderTarget = false;
    bool isShaderResource = true;
    ResourceStates initialState = ResourceStates::Unknown;
    bool keepInitialState = false;
    std::string debugName;
    
    constexpr TextureDesc& setWidth(uint32_t value) { width = value; return *this; }
    constexpr TextureDesc& setHeight(uint32_t value) { height = value; return *this; }
    constexpr TextureDesc& setFormat(Format value) { format = value; return *this; }
    constexpr TextureDesc& setIsRenderTarget(bool value) { isRenderTarget = value; return *this; }
    constexpr TextureDesc& setDebugName(const std::string& value) { debugName = value; return *this; }
    
    // Equivalent to .setInitialState(_initialState).setKeepInitialState(true)
    constexpr TextureDesc& enableAutomaticStateTracking(ResourceStates _initialState) {
        initialState = _initialState;
        keepInitialState = true;
        return *this;
    }
};

// Usage
TextureHandle texture = device->createTexture(
    TextureDesc()
        .setWidth(1024)
        .setHeight(1024)
        .setFormat(Format::RGBA8_UNORM)
        .setIsRenderTarget(true)
        .enableAutomaticStateTracking(ResourceStates::RenderTarget)
        .setDebugName("MyRenderTarget")
);
```

### Command Pattern

Commands are recorded for later execution:
```cpp
class CommandList {
public:
    void open() {
        // Prepare for recording
        m_isOpen = true;
    }
    
    void draw(uint32_t vertexCount, uint32_t startVertex) {
        // Record command (actual implementation varies by backend)
        if (isVulkan) {
            vkCmdDraw(m_commandBuffer, vertexCount, 1, startVertex, 0);
        } else if (isD3D12) {
            m_commandList->DrawInstanced(vertexCount, 1, startVertex, 0);
        }
    }
    
    void close() {
        // Finalize for execution
        m_isOpen = false;
    }
};

// Usage
CommandListHandle cmdList = device->createCommandList();
cmdList->open();
cmdList->setGraphicsState(state);
cmdList->draw(vertexCount, 0);
cmdList->close();
device->executeCommandList(cmdList);
```

### Type-Safe Casting

Template specialization for safe downcasting:
```cpp
template<typename T, typename U>
T checked_cast(U u) {
#ifdef _DEBUG
    T t = dynamic_cast<T>(u);
    assert(t && "Invalid type cast");
    return t;
#else
    return static_cast<T>(u);
#endif
}

// Usage in debug builds
VulkanDevice* vulkanDevice = checked_cast<VulkanDevice*>(device);
```

## Best Practices

1. **Always use RefCountPtr for resource ownership** to ensure proper lifetime management
2. **Call runGarbageCollection() regularly** - at least once per frame - to clean up released resources
3. **Prefer automatic resource state tracking** by using `keepInitialState = true` unless you have specific requirements
4. **Use the validation layer** during development to catch API misuse
5. **Check device capabilities** before using optional features with `queryFeatureSupport()`
6. **Use volatile constant buffers** for frequently updated small data (per-draw constants)
7. **Prefer setGraphicsState()** over individual state setters for better batching
8. **Use getNativeObject()** sparingly and only when absolutely necessary
9. **Always check return values** for creation functions - they return null on failure
10. **Use checked_cast** for downcasting interface pointers in debug builds
11. **Enable debug message callbacks** to receive errors and warnings during development
12. **Use descriptor tables** for mutable bindings that change frequently
13. **Use bindless layouts** for large, unbounded descriptor arrays
14. **Profile with GPU markers** using `beginMarker()`/`endMarker()` for debugging
15. **Consider memory layout differences** between APIs, especially for push constants and root parameters

## Common Patterns

### Resource Creation Pattern

```cpp
// 1. Create descriptor with chainable setters
TextureDesc desc;
desc.setWidth(size)
    .setHeight(size)
    .setFormat(format)
    .setIsRenderTarget(true)
    .enableAutomaticStateTracking(ResourceStates::RenderTarget)
    .setDebugName("MyRenderTarget");

// 2. Create resource
TextureHandle texture = device->createTexture(desc);

// 3. Check for success
if (!texture) {
    // Handle creation failure
    logError("Failed to create texture");
    return;
}
```

### Command Recording Pattern

```cpp
// 1. Create and open command list
CommandListHandle cmdList = device->createCommandList();
cmdList->open();

// 2. Set complete state in one call
GraphicsState state;
state.pipeline = graphicsPipeline;
state.framebuffer = framebuffer;
state.bindingSets = { bindingSet1, bindingSet2 };
state.viewport = viewportState;
state.rasterState = rasterState;
state.blendState = blendState;
state.depthStencilState = depthStencilState;

cmdList->setGraphicsState(state);

// 3. Issue commands
cmdList->clearTextureFloat(renderTarget, AllSubresources, Color(0.1f, 0.1f, 0.1f, 1.0f));

DrawArguments args;
args.vertexCount = vertexCount;
args.startVertex = 0;
cmdList->draw(args);

// 4. Close and execute
cmdList->close();
device->executeCommandList(cmdList);

// 5. Clean up (periodically, at least once per frame)
device->runGarbageCollection();
```

### Error Handling Pattern

```cpp
class MyMessageCallback : public IMessageCallback {
public:
    void message(MessageSeverity severity, const char* messageText) override {
        switch (severity) {
            case MessageSeverity::Fatal:
            case MessageSeverity::Error:
                logError(messageText);
                break;
            case MessageSeverity::Warning:
                logWarning(messageText);
                break;
            case MessageSeverity::Info:
                logInfo(messageText);
                break;
        }
    }
};

MyMessageCallback callback;
DeviceDesc desc;
desc.errorCB = &callback;
// ... configure desc ...

DeviceHandle device = nvrhi::vulkan::createDevice(desc);
if (!device) {
    logError("Failed to create device");
    return;
}
```

### Multi-Queue Rendering Pattern

```cpp
// Create command lists for different queues
CommandListHandle graphicsCmdList = device->createCommandList(
    CommandListParameters().setQueueType(CommandQueue::Graphics));
CommandListHandle computeCmdList = device->createCommandList(
    CommandListParameters().setQueueType(CommandQueue::Compute));

// Record graphics commands
graphicsCmdList->open();
// ... graphics commands ...
graphicsCmdList->close();

// Record compute commands
computeCmdList->open();
// ... compute commands ...
computeCmdList->close();

// Execute with synchronization
uint64_t graphicsInstance = device->executeCommandList(graphicsCmdList, CommandQueue::Graphics);
device->queueWaitForCommandList(CommandQueue::Compute, CommandQueue::Graphics, graphicsInstance);
uint64_t computeInstance = device->executeCommandList(computeCmdList, CommandQueue::Compute);
```

### Ray Tracing Pattern

```cpp
// Check ray tracing support
if (!device->queryFeatureSupport(Feature::RayTracingPipeline)) {
    // Fallback to rasterization
    return;
}

// Create acceleration structures
rt::AccelStructHandle blas = device->createAccelStruct(rt::AccelStructDesc()
    .setBottomLevel(true)
    .setDebugName("SceneBLAS"));

rt::AccelStructHandle tlas = device->createAccelStruct(rt::AccelStructDesc()
    .setBottomLevel(false)
    .setDebugName("SceneTLAS"));

// Build BLAS
CommandListHandle buildCmdList = device->createCommandList();
buildCmdList->open();

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

// Ray tracing dispatch
CommandListHandle rtCmdList = device->createCommandList();
rtCmdList->open();

rt::State rtState;
rtState.shaderTable = shaderTable;
rtState.bindings = { bindingSet };
rtCmdList->setRayTracingState(rtState);

rt::DispatchRaysArguments args;
args.width = width;
args.height = height;
args.depth = 1;
rtCmdList->dispatchRays(args);

rtCmdList->close();
device->executeCommandList(rtCmdList);
```

This coding style guide reflects the current state of the NVRHI codebase as of 2026. For the most up-to-date information, refer to the source code in the `include/nvrhi/` directory.
