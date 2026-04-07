# HLVM Engine RHI Object Guide

## Overview

This guide documents the high-level RHI (Rendering Hardware Interface) object wrappers built on top of NVRHI. These abstractions provide UE5-style C++ classes for managing graphics resources while maintaining clean separation from low-level API details (Vulkan/DX12).

**Key Benefits:**
- **RAII lifetime management** via smart pointers (`TUniquePtr`, `RefCountPtr`)
- **Backend abstraction** - same API works with Vulkan, DX11, or DX12
- **Automatic resource state tracking** via NVRHI's built-in mechanism
- **Thread-safe command list submission** across multiple queues

---

## Table of Contents

1. [FTexture](#ftexture) - Texture Resources
2. [FSampler](#fsampler) - Sampler State
3. [FFramebuffer](#fframebuffer) - Render Target Framebuffers
4. [FVertexBuffer / FIndexBuffer](#fvertexbuffer--findexbuffer) - Vertex/Index Buffers
5. [Complete Example](#complete-example) - Full Render Loop

---

## FTexture

### Purpose

`FTexture` wraps NVRHI texture resources with automatic view creation (RTV, DSV, SRV, UAV), sampler caching, and mip generation support.

### Key Methods

```cpp
// Initialization - Create empty texture
bool InitializeRenderTarget(
    TUINT32 Width,
    TUINT32 Height,
    ETextureFormat Format,
    nvrhi::IDevice* Device,
    TUINT32 InSampleCount = 1);  // MSAA sample count

// Upload data when initializing
bool Initialize(
    TUINT32 Width, TUINT32 Height, TUINT32 MipLevels, ETextureFormat Format,
    ETextureDimension Dimension,
    nvrhi::IDevice* Device,
    const void* InitialData = nullptr,  // Optional initial upload
    nvrhi::ICommandList* CommandList = nullptr);

// Resource views
[[nodiscard]] nvrhi::TextureHandle GetTextureHandle() const;   // Main handle
[[nodiscard]] nvrhi::TextureHandle GetTextureSRV() const;      // Shader Resource View
[[nodiscard]] nvrhi::TextureHandle GetTextureRTV() const;      // Render Target View
[[nodiscard]] nvrhi::TextureHandle GetTextureDSV() const;      // Depth Stencil View
[[nodiscard]] nvrhi::TextureHandle GetTextureUAV() const;      // Unordered Access View

// Update from CPU
void Update(const void* Data, TUINT32 DataSize, TUINT32 MipLevel, 
            TUINT32 ArraySlice, nvrhi::ICommandList* CommandList);

// Generate mips after upload
void GenerateMipmaps(nvrhi::ICommandList* CommandList);

// Sampling access
[[nodiscard]] nvrhi::SamplerHandle GetSampler(ETextureFilter Filter = ETextureFilter::Linear);
```

### Usage Example

```cpp
// Create render target texture for color output
auto ColorTexture = TUniquePtr<FTexture>(new FTexture());
ColorTexture->InitializeRenderTarget(
    WIDTH, HEIGHT,                     // Resolution
    ETextureFormat::RGBA8_UNORM,       // Format (see doc below)
    NvrhiDevice.Get(),                 // Device pointer
    1                                  // Sample count (1=non-MSAA, 4=4xMSAA)
);
ColorTexture->SetDebugName(TXT("ColorRenderTarget"));

// Create depth stencil texture
auto DepthTexture = TUniquePtr<FTexture>(new FTexture());
DepthTexture->InitializeRenderTarget(
    WIDTH, HEIGHT,
    ETextureFormat::D32,               // Deep precision
    NvrhiDevice.Get()
);
DepthTexture->SetDebugName(TXT("DepthRenderTarget"));

// If uploading initial data (e.g., loaded image):
uint8_t PixelData[WIDTH * HEIGHT * 4];  // RGBA pixels
Texture->Initialize(
    WIDTH, HEIGHT, 1, ETextureFormat::RGBA8_UNORM,
    ETextureDimension::Texture2D,
    Device,
    PixelData,              // Initial pixel data
    CommandList             // Upload happens here
);
```

### ETextureFormat Reference

Common formats (from `/Engine/Source/Runtime/Public/Renderer/RHI/Common.h`):

| Category | Formats | Description |
|----------|---------|-------------|
| **Color (8-bit)** | `RGBA8_UNORM`, `SBGRA8_UNORM`, `BGRA8_UNORM`, `Srgba8_UNorm` | Standard sRGB linear color |
| **Color (16-bit)** | `RGBA16_FLOAT`, `RGBA16_UNORM` | HDR floating-point or higher precision |
| **Color (32-bit)** | `RGBA32_FLOAT` | Full 32-bit float per channel (GPU compute) |
| **Depth** | `D32`, `D24S8`, `X24G8_UINT` | Depth-only or combined depth-stencil |
| **Compressed** | `BC1_UNORM`, `BC7_UNORM_SRGB`, etc. | GPU-compressed textures (smaller size) |

---

## FSampler

### Purpose

`FSampler` manages texture sampling state independently from textures, allowing dynamic changes between linear/mipmap/anisotropic filtering without recreating textures.

### Key Methods

```cpp
bool Initialize(
    ETextureFilter Filter,                        // Min/ Mag/ Mip filter mode
    ETextureAddress AddressU,                     // U address mode (Wrap/Clamp/Border)
    ETextureAddress AddressV,                     // V address mode
    ETextureAddress AddressW,                     // W address mode
    nvrhi::IDevice* Device,
    TFLOAT MaxAnisotropy = 16.0f);                // Anisotropy level (1.0 = disabled)

[[nodiscard]] nvrhi::SamplerHandle GetSamplerHandle() const;
```

### ETextureFilter Modes

- `ETextureFilter::Point` - Nearest neighbor sampling (no interpolation)
- `ETextureFilter::Linear` - Bilinear (linear min/mag, no mipmap)
- `ETextureFilter::Trilinear` - Trilinear (linear min/mag/mip)
- `ETextureFilter::LinearMipmapLinear` - Best quality (linear + mipmap blending)
- `ETextureFilter::Anisotropic` - Anisotropic filtering (best edge quality at cost)

### ETextureAddress Modes

- `ETextureAddress::Wrap` - Tile texture infinitely
- `ETextureAddress::Clamp` - Clamp to edge pixels
- `ETextureAddress::Mirror` - Mirror pattern
- `ETextureAddress::Border` - Use border color (requires border samples in shader)

### Usage Example

```cpp
// High-quality anisotropic sampler for diffuse textures
FSampler DiffuseSampler;
DiffuseSampler.Initialize(
    ETextureFilter::Anisotropic,      // Highest quality
    ETextureAddress::Wrap,            // Tile for textures
    ETextureAddress::Wrap,
    ETextureAddress::Wrap,
    Device,
    16.0f                             // 16x anisotropy
);

// Point sampler for pixel art (no blur)
FSampler PixelSampler;
PixelSampler.Initialize(
    ETextureFilter::Point,
    ETextureAddress::Clamp,           // Clamp edges for clean borders
    ETextureAddress::Clamp,
    ETextureAddress::Clamp,
    Device
);
```

---

## FFramebuffer

### Purpose

`FFramebuffer` manages render target attachments and viewport/scissor state for rendering passes. Think of it as "where to draw next."

### Key Methods

```cpp
// Lifecycle
bool Initialize(nvrhi::IDevice* InDevice);         // Must call first
bool CreateFramebuffer();                          // Actually create NVRHI handle

// Attachment management
void AddColorAttachment(const FFramebufferAttachment& Attachment);
void AddColorAttachment(nvrhi::TextureHandle Texture, TUINT32 MipLevel = 0, TUINT32 ArraySlice = 0);

void SetDepthAttachment(const FFramebufferAttachment& Attachment);
void SetDepthAttachment(nvrhi::TextureHandle Texture, TUINT32 MipLevel = 0, TUINT32 ArraySlice = 0);

// Viewport/scissor configuration (called before render loop)
void SetViewport(TFLOAT X, TFLOAT Y, TFLOAT Width, TFLOAT Height, TFLOAT MinDepth, TFLOAT MaxDepth);
void SetScissor(TINT32 X, TINT32 Y, TUINT32 Width, TUINT32 Height);

// Accessors
[[nodiscard]] nvrhi::FramebufferHandle GetFramebufferHandle() const;
[[nodiscard]] TUINT32 GetWidth() const;
[[nodiscard]] TUINT32 GetHeight() const;
[[nodiscard]] TUINT32 GetColorAttachmentCount() const;
[[nodiscard]] nvrhi::Viewport GetViewport() const;
[[nodiscard]] nvrhi::Rect GetScissor() const;
```

### FFramebufferAttachment Detail

```cpp
struct FFramebufferAttachment
{
    nvrhi::TextureHandle Texture;   // Attach this texture
    TUINT32 MipLevel;               // Which mip level (usually 0)
    TUINT32 ArraySlice;             // Which array slice/cube face (usually 0)
    TUINT32 SampleCount;            // MSAA sample count
};
```

### Usage Example

```cpp
// Configure one-time framebuffer setup
FFramebuffer MainFramebuffer;
MainFramebuffer.Initialize(Device);

// Add color attachment from FTexture
MainFramebuffer.AddColorAttachment(ColorTexture->GetTextureSRV());

// Add depth attachment
MainFramebuffer.SetDepthAttachment(DepthTexture->GetTextureDSV());

// Actually create the framebuffer handle
MainFramebuffer.CreateFramebuffer();
MainFramebuffer.SetDebugName(TXT("MainPassFramebuffer"));

// Setup viewport (do this every frame before drawing if resolution changes)
MainFramebuffer.SetViewport(
    0.0f, 0.0f,                    // Top-left corner
    WIDTH, HEIGHT,                  // Dimensions
    0.0f, 1.0f                     // Z range (standard 0-1 for Vulkan/DX12)
);

// Clip rendering to specific region
MainFramebuffer.SetScissor(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
```

### Using in Render Pass

```cpp
// In your render loop:
NvrhiCommandList->open();

// Begin render pass - tells GPU framebuffer layout
nvrhi::PassDesc PassDesc;
PassDesc.colorAttachments[0].loadOp = nvrhi::LoadOp::Clear;
PassDesc.colorAttachments[0].storeOp = nvrhi::StoreOp::Store;
PassDesc.colorAttachments[0].clearValue = nvrhi::Color(0.1f, 0.1f, 0.1f, 1.0f);  // Clear color

PassDesc.depthAttachment.loadOp = nvrhi::LoadOp::Clear;
PassDesc.depthAttachment.storeOp = nvrhi::StoreOp::Store;
PassDesc.depthAttachment.clearValue.depth = 1.0f;

NvrhiCommandList->beginRenderPass(PassDesc, MainFramebuffer.GetFramebufferHandle());

// Set graphics state
nvrhi::GraphicsState State;
State.pipeline = GraphicsPipeline.Get();
State.framebuffer = MainFramebuffer.GetFramebufferHandle();
State.bindingSets = { BindingSet };
State.viewport = MainFramebuffer.GetViewport();
State.rasterState = RasterState;

NvrhiCommandList->setGraphicsState(State);

// Draw commands...

// End pass
NvrhiCommandList->endRenderPass();
NvrhiCommandList->close();
```

---

## FVertexBuffer / FIndexBuffer

### Two Buffer Strategy

The engine provides **static** and **dynamic** variants based on usage patterns:

| Type | Lifecycle | Memory Model | Update Method | Performance |
|------|-----------|--------------|---------------|-------------|
| **Static** | Created once, read-only | GPU-only memory | `cmd->writeBuffer()` | Fastest |
| **Dynamic** | Frequent updates | CPU-visible memory | `map()/unmap()` or `update()` | Flexible |

### Static Vertex Buffer (`FStaticVertexBuffer`)

Use for: Character meshes, scene geometry, UI elements that don't change

```cpp
class FStaticVertexBuffer : public FVertexBuffer
{
public:
    bool Initialize(
        nvrhi::ICommandList* CommandList,
        nvrhi::IDevice* Device,
        const void* VertexData,     // All vertex data
        size_t VertexDataSize);     // Total bytes
};
```

### Dynamic Vertex Buffer (`FDynamicVertexBuffer`)

Use for: Animated bone skinned meshes (bone transforms update), particle systems, procedural geometry

```cpp
class FDynamicVertexBuffer : public FVertexBuffer
{
public:
    // Allocate blank buffer (no initial data)
    bool Initialize(nvrhi::IDevice* Device, size_t BufferSize);

    // Option 1: Map/unmap (faster but requires synchronization)
    void* Map(nvrhi::CpuAccessMode AccessMode = nvrhi::CpuAccessMode::Write);
    void  Unmap();

    // Option 2: Command list submit (safer, auto-synced)
    void Update(
        nvrhi::ICommandList* CommandList,
        const void* Data,
        size_t DataSize,
        size_t DstOffset = 0);     // Offset within buffer
    
    [[nodiscard]] bool IsMapped() const;
};
```

### Static Index Buffer (`FStaticIndexBuffer`)

```cpp
class FStaticIndexBuffer : public FIndexBuffer
{
public:
    bool Initialize(
        nvrhi::ICommandList* CommandList,
        nvrhi::IDevice* Device,
        const void* IndexData,      // Index array
        size_t IndexDataSize,
        nvrhi::Format IndexFormat); // e.g., `nvrhi::Format::R32_UINT`
};
```

### Index Format Reference

| Format | Size | Range | Use Case |
|--------|------|-------|----------|
| `R16_UINT` | 2 bytes | 0-65,535 | Most meshes (<64k vertices) |
| `R32_UINT` | 4 bytes | 0-4B+ | Large scenes (>64k vertices) |

### Usage Example

```cpp
// Define vertex structure
struct FVertex
{
    float Position[3];  // XYZ position
    float Color[3];     // RGB color
    float Normal[3];    // Surface normal (for lighting)
};

// Static triangle mesh
const FVertex Vertices[] = {
    { { 0.0f,  0.8f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0, 0, 1 } },
    { { -0.8f, -0.8f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0, 0, 1 } },
    { { 0.8f, -0.8f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0, 0, 1 } }
};

StaticVertexBuffer->Initialize(
    CommandList,        // Upload during command recording
    Device,
    Vertices,
    sizeof(Vertices)
);
StaticVertexBuffer->SetDebugName(TXT("TriangleMesh"));

// Static indices
const uint32_t Indices[] = { 0, 1, 2 };

StaticIndexBuffer->Initialize(
    CommandList,
    Device,
    Indices,
    sizeof(Indices),
    nvrhi::Format::R32_UINT
);
StaticIndexBuffer->SetDebugName(TXT("TriangleIndices"));

// Bind in render loop:
CmdList->bindVertexBuffers(0, &StaticVertexBuffer->GetBufferHandle(), 1, 0);
CmdList->bindIndexBuffer(StaticIndexBuffer->GetBufferHandle(), 0, nvrhi::Format::R32_UINT);
```

### Dynamic Buffer with Map/Unmap

```cpp
// For per-frame animation data
auto DynVB = TUniquePtr<FDynamicVertexBuffer>(new FDynamicVertexBuffer());
DynVB->Initialize(Device, sizeof(AnimationData));

// Every frame:
auto AnimData = CalculateFrameAnimation();  // Your animation logic

// Lock buffer (will stall until GPU is done)
float* LockedData = static_cast<float*>(DynVB->Map());

// Copy new data
memcpy(LockedData, &AnimData, sizeof(AnimData));

// Unlock (submit to GPU queue)
DynVB->Unmap();
```

---

## Complete Example

Based on `/Engine/Source/Runtime/Test/TestRHIObjects.cpp`:

### Context Structure

```cpp
struct FRHITestContext
{
    // Window and Vulkan handles (simplified)
    glfw::Window* Window;
    vk::Instance Instance;
    vk::PhysicalDevice PhysicalDevice;
    vk::Device Device;
    vk::Queue GraphicsQueue;
    
    // NVRHI device and command lists
    nvrhi::DeviceHandle NvrhiDevice;
    nvrhi::CommandListHandle NvrhiCommandList;
    
    // HLVM RHI Objects
    TUniquePtr<FTexture> ColorTexture;
    TUniquePtr<FTexture> DepthTexture;
    TUniquePtr<FFramebuffer> Framebuffer;
    TUniquePtr<FVertexBuffer> VertexBuffer;
    TUniquePtr<FIndexBuffer> IndexBuffer;
    
    // Synchronization primitives
    vector<vk::UniqueSemaphore> ImageAvailableSemaphores;
    vector<vk::UniqueSemaphore> RenderFinishedSemaphores;
    vector<vk::UniqueFence> InFlightFences;
    size_t CurrentFrame = 0;
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
};
```

### Resource Creation Phase

```cpp
void CreateRHIResources(FRHITestContext& Context)
{
    // Create NVRHI device wrapper from Vulkan handles
    nvrhi::vulkan::DeviceDesc DeviceDesc;
    DeviceDesc.errorCB = nullptr;
    DeviceDesc.instance = Context.Instance;
    DeviceDesc.physicalDevice = Context.PhysicalDevice;
    DeviceDesc.device = Context.Device;
    DeviceDesc.graphicsQueue = Context.GraphicsQueue;
    DeviceDesc.graphicsQueueIndex = 0;
    
    Context.NvrhiDevice = nvrhi::vulkan::createDevice(DeviceDesc);
    if (!Context.NvrhiDevice)
        throw runtime_error("Failed to create NVRHI device");
    
    // Create command list with upload buffer optimization
    nvrhi::CommandListParameters params = {};
    params.enableImmediateExecution = false;
    params.setUploadChunkSize((size_t)(1 * 1024 * 1024));  // 1MB staging
    
    Context.NvrhiCommandList = Context.NvrhiDevice->createCommandList(params);
    
    // Open for recording
    Context.NvrhiCommandList->open();
    
    // --- Create render target textures ---
    
    // Color attachment (what we'll draw into screen)
    Context.ColorTexture = TUniquePtr<FTexture>(new FTexture());
    Context.ColorTexture->InitializeRenderTarget(
        WIDTH, HEIGHT, ETextureFormat::RGBA8_UNORM, Context.NvrhiDevice.Get()
    );
    Context.ColorTexture->SetDebugName(TXT("ColorRenderTarget"));
    
    // Depth attachment (what stores Z values)
    Context.DepthTexture = TUniquePtr<FTexture>(new FTexture());
    Context.DepthTexture->InitializeRenderTarget(
        WIDTH, HEIGHT, ETextureFormat::D32, Context.NvrhiDevice.Get()
    );
    Context.DepthTexture->SetDebugName(TXT("DepthRenderTarget"));
    
    // --- Create framebuffer (glues textures together) ---
    
    Context.Framebuffer = TUniquePtr<FFramebuffer>(new FFramebuffer());
    Context.Framebuffer->Initialize(Context.NvrhiDevice.Get());
    Context.Framebuffer->AddColorAttachment(Context.ColorTexture->GetTextureRTV());
    Context.Framebuffer->SetDepthAttachment(Context.DepthTexture->GetTextureDSV());
    Context.Framebuffer->CreateFramebuffer();
    Context.Framebuffer->SetViewport(0, 0, WIDTH, HEIGHT, 0, 1);
    Context.Framebuffer->SetScissor(0, 0, WIDTH, HEIGHT);
    
    // --- Create static triangle mesh ---
    
    struct FVertex { float Position[3]; float Color[3]; };
    FVertex Vertices[] = {
        { { 0.0f, 0.8f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { -0.8f, -0.8f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        { { 0.8f, -0.8f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
    };
    
    uint32_t Indices[] = { 0, 1, 2 };
    
    // For dynamic buffers:
    #if TEST_DYNAMIC_BUFFER
    Context.VertexBuffer = TUniquePtr<FDynamicVertexBuffer>(new FDynamicVertexBuffer());
    static_cast<FDynamicVertexBuffer*>(Context.VertexBuffer.get())->Initialize(
        Context.NvrhiDevice.Get(), sizeof(Vertices)
    );
    static_cast<FDynamicVertexBuffer*>(Context.VertexBuffer.get())->Update(
        Context.NvrhiCommandList, Vertices, sizeof(Vertices)
    );
    
    Context.IndexBuffer = TUniquePtr<FDynamicIndexBuffer>(new FDynamicIndexBuffer());
    static_cast<FDynamicIndexBuffer*>(Context.IndexBuffer.get())->Initialize(
        Context.NvrhiDevice.Get(), sizeof(Indices), nvrhi::Format::R32_UINT
    );
    static_cast<FDynamicIndexBuffer*>(Context.IndexBuffer.get())->Update(
        Context.NvrhiCommandList, Indices, sizeof(Indices)
    );
    
    #else
    // For static meshes (most common):
    Context.VertexBuffer = TUniquePtr<FStaticVertexBuffer>(new FStaticVertexBuffer());
    static_cast<FStaticVertexBuffer*>(Context.VertexBuffer.get())->Initialize(
        Context.NvrhiCommandList, Context.NvrhiDevice.Get(), Vertices, sizeof(Vertices)
    );
    Context.VertexBuffer->SetDebugName(TXT("StaticTriangleVertexBuffer"));
    
    Context.IndexBuffer = TUniquePtr<FStaticIndexBuffer>(new FStaticIndexBuffer());
    static_cast<FStaticIndexBuffer*>(Context.IndexBuffer.get())->Initialize(
        Context.NvrhiCommandList, Context.NvrhiDevice.Get(), Indices, sizeof(Indices), 
        nvrhi::Format::R32_UINT
    );
    Context.IndexBuffer->SetDebugName(TXT("StaticTriangleIndexBuffer"));
    #endif
    
    // Close and submit
    Context.NvrhiCommandList->close();
    Context.NvrhiDevice->executeCommandList(Context.NvrhiCommandList);
}
```

### Cleanup Pattern

```cpp
void CleanupRHIResources(FRHITestContext& Context)
{
    // RHI Objects will auto-release via RAII
    Context.IndexBuffer.reset();
    Context.VertexBuffer.reset();
    Context.Framebuffer.reset();
    Context.DepthTexture.reset();
    Context.ColorTexture.reset();
    
    // NVRHI handles also reset
    if (Context.NvrhiCommandList)
        Context.NvrhiCommandList.Reset();
    
    if (Context.NvrhiDevice)
        Context.NvrhiDevice.Reset();
}
```

### Render Loop Integration

```cpp
// Inside game loop:
glfwPollEvents();

// Wait for previous frame to finish
vk::Fence fence = Context.InFlightFences[Context.CurrentFrame];
Context.Device.waitForFences(fence, VK_TRUE, UINT64_MAX);

// Acquire next swapchain image
auto ImageIndex = Context.Device.acquireNextImageKHR(Context.Swapchain, UINT64_MAX);

// Reset fence for this frame
Context.Device.resetFences(fence);

// Record command list for this frame
Context.NvrhiCommandList->open();

// Set up binding sets with descriptor tables...
// NVRHI's automatic barrier placement handles resource states

// Render
Context.NvrhiCommandList->beginRenderPass(ClearPass, Context.Framebuffer->GetFramebufferHandle());
Context.NvrhiCommandList->setGraphicsState(GraphicsState);
Context.NvrhiCommandList->drawIndexed(IndexArgs);
Context.NvrhiCommandList->endRenderPass();

// Present
Context.NvrhiCommandList->close();
Context.Device.waitForIdle();  // Wait for GPU before presenting
PresentCommandList->execute();
```

---

## Best Practices

### 1. **Lifecycle Management**

- Always use `TUniquePtr<T>` for ownership transfer
- Call `reset()` explicitly before context destruction
- NVRHI handles need `.Reset()` method (COM-style ref counting)

```cpp
// BAD: Manual delete (memory leak if exception thrown)
FTexture* Texture = new FTexture();
delete Texture;

// GOOD: RAII with smart pointer
auto Texture = TUniquePtr<FTexture>(new FTexture());
// Auto-deleted when scope ends
```

### 2. **Command List Patterns**

```cpp
// RECOMMENDED: Recreate command lists per-frame (thread safe)
// OR reuse with proper sync

// BAD: Reusing stale command lists without checking execution status
```

### 3. **Resource State Awareness**

- Enable automatic state tracking via `enableAutomaticStateTracking()` in `TextureDesc`
- NVRHI inserts barriers automatically between state transitions
- Disable only for performance-critical tight loops where you know exact states

### 4. **Error Handling**

```cpp
try
{
    CreateRHIResources(Context);
}
catch (const exception& e)
{
    cerr << "Fatal Error: " << e.what() << endl;
    CleanupRHIResources(Context);  // Don't forget cleanup!
    return false;
}
```

### 5. **Validation Layers**

```cpp
bool enableValidationLayers = !HLVM_BUILD_RELEASE;  // Always on in debug

if (enableValidationLayers)
{
    Layers.push_back("VK_LAYER_KHRONOS_validation");
    Extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
}
```

### 6. **Staging Buffer Optimization**

```cpp
// Prevent Vulkan staging fragmentation
params.setUploadChunkSize(1ULL * 1024 * 1024);  // 1MB chunks
```

---

## Common Pitfalls

### Pitfall 1: Forgot to Reset Handles

Vulkan resources must be destroyed before creating new ones in some backends. NVRHI handles handle this automatically, but manual `vk::Unique*` handles must be released.

```cpp
// WRONG: Holding raw pointers
vector<vk::Semaphore> RenderFinishedSemaphores;  // Will leak

// RIGHT: Using unique handles
vector<vk::UniqueSemaphore> RenderFinishedSemaphores;
// Auto-releases when cleared
```

### Pitfall 2: Wrong Clear Values

Depth value defaults are backend-specific. Always set explicitly:

```cpp
PassDesc.depthAttachment.clearValue.depth = 1.0f;  // Far plane
PassDesc.depthAttachment.clearValue.stencil = 0;   // Optional
```

### Pitfall 3: Missing Fence/Semaphore Sync

```cpp
// BEFORE drawing, wait for previous frame:
device.waitForFences(fence[frame], VK_TRUE, ~0ULL);
device.resetFences(fence[frame]);

// AFTER draw, signal semaphore for present:
device.submit(semaphores[frame], fence[frame]);
```

---

## Related Documentation

- NVRHI API Reference: `/Vibe_Coding/NVRHI_API/v2/DOC_api_reference.md`
- NVRHI Tutorial: `/home/hangyu5/Documents/Gitrepo-Other/RHI/RBDOOM-3-BFG/neo/extern/nvrhi/doc/Tutorial.md`
- DeviceManager Implementation: `/home/hangyu5/Documents/Gitrepo-Other/RHI/Donut-Samples/Donut/include/donut/app/DeviceManager.h`
- Test File (Reference): `/Engine/Source/Runtime/Test/TestRHIObjects.cpp`

---

## Version History

| Date | Notes |
|------|-------|
| 2025-XX-XX | Initial documentation |
| 2026-03-01 | Added best practices and pitfall section |

---

*End of Guide*
