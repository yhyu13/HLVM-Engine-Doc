# DOOM 3 NVRHI Architecture Documentation

This document provides a comprehensive analysis of the DOOM 3 BFG Edition's NVRHI implementation, focusing on the DeviceManager architecture and NVRHI adapter patterns. This knowledge will be used to implement similar systems in the HLVM-Engine.

## Overview

The DOOM 3 BFG Edition uses NVRHI (NVIDIA Rendering Hardware Interface) as a cross-platform graphics abstraction layer that sits on top of DirectX 11, DirectX 12, and Vulkan. This allows the engine to support multiple graphics APIs with a single codebase.

## Architecture Components

### 1. DeviceManager Architecture

#### Core Class: `DeviceManager`

The `DeviceManager` class serves as the central abstraction for graphics device management. It's located in `/neo/sys/DeviceManager.h` and `/neo/sys/DeviceManager.cpp`.

**Key Features:**
- Cross-platform graphics API abstraction (D3D11, D3D12, Vulkan)
- Window management integration
- Swap chain management
- Frame buffer management
- Device parameter configuration

#### Factory Pattern Implementation

```cpp
class DeviceManager {
public:
    static DeviceManager* Create(nvrhi::GraphicsAPI api);
    
private:
    static DeviceManager* CreateD3D11();
    static DeviceManager* CreateD3D12();
    static DeviceManager* CreateVK();
};
```

The factory pattern allows runtime selection of the graphics API based on configuration or capabilities.

#### Device Creation Parameters

The `DeviceCreationParameters` structure provides comprehensive configuration for device initialization:

```cpp
struct DeviceCreationParameters {
    // Window configuration
    bool startMaximized = false;
    bool startFullscreen = false;
    int windowPosX = -1;
    int windowPosY = -1;
    uint32_t backBufferWidth = 1280;
    uint32_t backBufferHeight = 720;
    
    // Swap chain configuration
    uint32_t backBufferSampleCount = 1;
    uint32_t refreshRate = 0;
    uint32_t swapChainBufferCount = NUM_FRAME_DATA;
    nvrhi::Format swapChainFormat = nvrhi::Format::RGBA8_UNORM;
    
    // Debug and validation
    bool enableDebugRuntime = false;
    bool enableNvrhiValidationLayer = false;
    int vsyncEnabled = 0;
    
    // Feature support
    bool enableRayTracingExtensions = false;
    bool enableComputeQueue = false;
    bool enableCopyQueue = false;
    
    // Platform-specific handles
#if _WIN32
    HINSTANCE hInstance;
    HWND hWnd;
#endif
    
    // Adapter selection
    IDXGIAdapter* adapter = nullptr;
    std::wstring adapterNameSubstring = L"";
    
    // DPI awareness
    bool enablePerMonitorDPI = false;
    
    // API-specific configurations
#if USE_DX11 || USE_DX12
    DXGI_USAGE swapChainUsage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
#endif

#if USE_VK
    std::vector<std::string> requiredVulkanInstanceExtensions;
    std::vector<std::string> requiredVulkanDeviceExtensions;
    std::vector<std::string> requiredVulkanLayers;
    // ... more Vulkan-specific options
#endif
};
```

#### Virtual Interface Design

The `DeviceManager` uses pure virtual methods for API-specific implementations:

```cpp
class DeviceManager {
protected:
    virtual bool CreateDeviceAndSwapChain() = 0;
    virtual void DestroyDeviceAndSwapChain() = 0;
    virtual void ResizeSwapChain() = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Present() = 0;

public:
    [[nodiscard]] virtual nvrhi::IDevice* GetDevice() const = 0;
    [[nodiscard]] virtual const char* GetRendererString() const = 0;
    [[nodiscard]] virtual nvrhi::GraphicsAPI GetGraphicsAPI() const = 0;
    
    virtual nvrhi::ITexture* GetCurrentBackBuffer() = 0;
    virtual nvrhi::ITexture* GetBackBuffer(uint32_t index) = 0;
    virtual uint32_t GetCurrentBackBufferIndex() = 0;
    virtual uint32_t GetBackBufferCount() = 0;
};
```

### 2. NVRHI Adapter Implementation

The NVRHI adapter layer is implemented in `/neo/renderer/NVRHI/` with the following key files:

#### Core Components:

1. **RenderBackend_NVRHI.cpp** - Main rendering backend implementation
2. **Image_NVRHI.cpp** - Texture and image management
3. **BufferObject_NVRHI.cpp** - Vertex/index buffer management
4. **Framebuffer_NVRHI.cpp** - Framebuffer operations
5. **RenderProgs_NVRHI.cpp** - Shader program management
6. **RenderDebug_NVRHI.cpp** - Debug rendering utilities

#### Key Architecture Patterns:

##### 1. Resource Management

**Image/Texture Management:**
```cpp
class idImage {
private:
    nvrhi::TextureHandle texture;
    nvrhi::SamplerDesc samplerDesc;
    
public:
    void CreateSampler();
    void AllocImage();
    void Bind();
};
```

**Buffer Management:**
```cpp
class idVertexBuffer {
private:
    nvrhi::BufferHandle bufferHandle;
    
public:
    bool AllocBufferObject(const void* data, int allocSize, bufferUsageType_t usage, nvrhi::ICommandList* commandList);
};
```

##### 2. Rendering State Management

**Binding Layout System:**
The implementation uses a sophisticated binding layout system to handle different shader binding requirements:

```cpp
// Different binding layout types for various shader configurations
enum {
    BINDING_LAYOUT_DEFAULT,
    BINDING_LAYOUT_DEFAULT_SKINNED,
    BINDING_LAYOUT_GBUFFER,
    BINDING_LAYOUT_GBUFFER_SKINNED,
    BINDING_LAYOUT_TEXTURE,
    BINDING_LAYOUT_TEXTURE_SKINNED,
    BINDING_LAYOUT_AMBIENT_LIGHTING_IBL,
    BINDING_LAYOUT_DRAW_INTERACTION,
    BINDING_LAYOUT_DRAW_INTERACTION_SKINNED,
    // ... more layouts
};

void idRenderBackend::GetCurrentBindingLayout(int type) {
    auto& desc = pendingBindingSetDescs[type];
    
    // Configure bindings based on type
    nvrhi::IBuffer* paramCb = renderProgManager.ConstantBuffer();
    
    if (type == BINDING_LAYOUT_DEFAULT) {
        desc[0].bindings = {
            nvrhi::BindingSetItem::ConstantBuffer(0, paramCb, nvrhi::EntireBuffer)
        };
        desc[1].bindings = {
            nvrhi::BindingSetItem::Texture_SRV(0, GetImageAt(0)->GetTextureID())
        };
        desc[2].bindings = {
            nvrhi::BindingSetItem::Sampler(0, GetImageAt(0)->GetSampler(samplerCache))
        };
    }
    // ... handle other layout types
}
```

##### 3. Pipeline State Caching

```cpp
void idRenderBackend::DrawElementsWithCounters(const drawSurf_t* surf, bool shadowCounter) {
    // Get current binding layout
    const int bindingLayoutType = renderProgManager.BindingLayoutType();
    
    // Get cached pipeline
    const PipelineKey key{ glStateBits, program, static_cast<int>(depthBias), slopeScaleBias, currentFrameBuffer };
    const auto pipeline = pipelineCache.GetOrCreatePipeline(key);
    
    // Set up graphics state
    if (changeState) {
        nvrhi::GraphicsState state;
        state.bindings.push_back(currentBindingSets[i]);
        state.indexBuffer = { currentIndexBuffer, nvrhi::Format::R16_UINT, 0 };
        state.vertexBuffers = { { currentVertexBuffer, 0, 0 } };
        state.pipeline = pipeline;
        state.framebuffer = currentFrameBuffer->GetApiObject();
        
        commandList->setGraphicsState(state);
        renderProgManager.CommitPushConstants(commandList, bindingLayoutType);
    }
    
    // Issue draw call
    nvrhi::DrawArguments args;
    args.startVertexLocation = currentVertexOffset / sizeof(idDrawVert);
    args.startIndexLocation = currentIndexOffset / sizeof(triIndex_t);
    args.vertexCount = surf->numIndexes;
    commandList->drawIndexed(args);
}
```

##### 4. Frame Management

```cpp
void idRenderBackend::BeginFrame() {
    deviceManager->BeginFrame();
    commandList->open();
}

void idRenderBackend::EndFrame() {
    commandList->close();
    deviceManager->ExecuteCommandList(commandList);
    deviceManager->Present();
}
```

### 3. Platform Integration

#### Window Management

The DeviceManager integrates with platform-specific window systems:

```cpp
bool DeviceManager::CreateWindowDeviceAndSwapChain(const glimpParms_t& params, const char* windowTitle) {
    // Platform-specific window creation
    // Initialize graphics device
    // Create swap chain
    return true;
}

void DeviceManager::UpdateWindowSize(const glimpParms_t& params) {
    BackBufferResizing();
    m_DeviceParams.backBufferWidth = params.width;
    m_DeviceParams.backBufferHeight = params.height;
    ResizeSwapChain();
    BackBufferResized();
}
```

#### DPI Awareness

```cpp
void DeviceManager::GetDPIScaleInfo(float& x, float& y) const {
    x = m_DPIScaleFactorX;
    y = m_DPIScaleFactorY;
}
```

### 4. Error Handling and Debugging

#### Message Callback System

```cpp
struct DefaultMessageCallback : public nvrhi::IMessageCallback {
    static DefaultMessageCallback& GetInstance();
    void message(nvrhi::MessageSeverity severity, const char* messageText) override;
};

void DefaultMessageCallback::message(nvrhi::MessageSeverity severity, const char* messageText) {
    switch (severity) {
        case nvrhi::MessageSeverity::Info:
            common->Printf(messageText);
            break;
        case nvrhi::MessageSeverity::Warning:
            common->Warning(messageText);
            break;
        case nvrhi::MessageSeverity::Error:
            common->FatalError("%s", messageText);
            break;
        case nvrhi::MessageSeverity::Fatal:
            common->FatalError("%s", messageText);
            break;
    }
}
```

## Key Design Principles

### 1. Abstraction Layering

- **Platform Layer**: Handles OS-specific windowing and input
- **DeviceManager Layer**: Abstracts graphics API differences
- **NVRHI Layer**: Provides unified rendering interface
- **Engine Layer**: Game logic and rendering commands

### 2. Resource Lifetime Management

- Automatic resource cleanup through RAII patterns
- Frame-based resource management for dynamic resources
- GPU memory management integration (VMA for Vulkan)

### 3. Performance Optimization

- Pipeline state caching to minimize state changes
- Binding layout optimization
- Frame-based command buffer management
- Memory pool management

### 4. Extensibility

- Virtual interface design for new API support
- Plugin-style architecture for features
- Configuration-driven feature enabling

## Integration Points

### 1. Application Initialization

```cpp
void idRenderBackend::Init() {
    // Create device manager
    nvrhi::GraphicsAPI api = nvrhi::GraphicsAPI::D3D12;
    if (!idStr::Icmp(r_graphicsAPI.GetString(), "vulkan")) {
        api = nvrhi::GraphicsAPI::VULKAN;
    }
    deviceManager = DeviceManager::Create(api);
    
    // Initialize window and device
    R_SetNewMode(true);
    
    // Initialize rendering components
    renderProgManager.Init(deviceManager->GetDevice());
    bindingCache.Init(deviceManager->GetDevice());
    samplerCache.Init(deviceManager->GetDevice());
    pipelineCache.Init(deviceManager->GetDevice());
    
    // Create command list
    commandList = deviceManager->GetDevice()->createCommandList(params);
}
```

### 2. Resource Creation Flow

```cpp
// Buffer creation
nvrhi::BufferDesc desc;
desc.byteSize = size;
desc.isVertexBuffer = true;
desc.initialState = nvrhi::ResourceStates::CopyDest;
bufferHandle = deviceManager->GetDevice()->createBuffer(desc);

// Texture creation
nvrhi::TextureDesc texDesc;
texDesc.width = width;
texDesc.height = height;
texDesc.format = nvrhi::Format::RGBA8_UNORM;
texDesc.isRenderTarget = true;
textureHandle = deviceManager->GetDevice()->createTexture(texDesc);
```

## Best Practices Observed

1. **Consistent Error Handling**: All API calls are checked for errors
2. **Resource Naming**: Debug names are assigned to all resources
3. **State Validation**: Proper state validation before API calls
4. **Memory Alignment**: Proper memory alignment for performance
5. **Thread Safety**: Proper synchronization for multi-threaded rendering
6. **Platform Abstraction**: Clean separation of platform-specific code

## Conclusion

The DOOM 3 NVRHI implementation demonstrates a well-architected graphics abstraction layer that successfully unifies multiple graphics APIs under a single interface. The key to its success lies in:

1. **Clear abstraction boundaries**
2. **Comprehensive parameter configuration**
3. **Efficient resource management**
4. **Performance-conscious design**
5. **Extensible architecture**

These principles will guide the implementation of our own DeviceManager and NVRHI adapter for the HLVM-Engine.