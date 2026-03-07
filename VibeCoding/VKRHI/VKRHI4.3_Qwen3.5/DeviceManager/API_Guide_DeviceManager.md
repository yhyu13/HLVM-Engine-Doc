# FDeviceManager Usage Guide

## Overview

`FDeviceManager` is a high-level abstraction for managing window creation, graphics device initialization, and swap chain management in the HLVM-Engine. It provides a unified interface for different graphics APIs while handling complex Vulkan details like queue family selection, extension management, and synchronization.

**Location**: `Engine/Source/Runtime/Public/Renderer/DeviceManager.h`  
**Vulkan Implementation**: `Engine/Source/Runtime/Private/Renderer/DeviceManagerVk.cpp`

---

## Quick Start Example

Here's a minimal example of using `FDeviceManager` to create a Vulkan device and render:

```cpp
#include "Renderer/DeviceManager.h"

// 1. Get DeviceManager instance
TUniquePtr<FDeviceManager> DeviceManager = FDeviceManager::Create(nvrhi::GraphicsAPI::VULKAN);
if (!DeviceManager)
{
    HLVM_LOG(LogRHI, err, TXT("Failed to create device manager"));
    return false;
}

// 2. Configure creation parameters (modify BEFORE creating window/device)
IWindow::Properties WindowProps;
WindowProps.Title = TEXT("My Application");
WindowProps.Extent = {800, 600};
WindowProps.Resizable = true;
WindowProps.VSync = IWindow::EVsync::Off;

FDeviceCreationParameters& Params = const_cast<FDeviceCreationParameters&>(DeviceManager->GetDeviceParams());
Params.BackBufferWidth = 800;
Params.BackBufferHeight = 600;
Params.SwapChainBufferCount = 2; // Double buffering
Params.bEnableDebugRuntime = HLVM_BUILD_DEBUG;
Params.bEnableNVRHIValidationLayer = HLVM_BUILD_DEBUG;

// 3. Create window + device + swapchain
if (!DeviceManager->CreateWindowDeviceAndSwapChain(WindowProps))
{
    HLVM_LOG(LogRHI, err, TXT("Failed to initialize device"));
    return false;
}

HLVM_LOG(LogRHI, info, TXT("Successfully created device: {}"), 
         FString(DeviceManager->GetRendererString()));

// 4. Render loop
int FrameCount = 0;
while (FrameCount < 100)
{
    // Acquire swapchain image
    if (!DeviceManager->BeginFrame())
        break;
    
    // ... Your rendering code here ...
    // Use DeviceManager->GetCurrentFramebuffer() for current framebuffer
    // Use DeviceManager->GetDevice() to get nvrhi::IDevice*
    
    // Signal frame completion
    DeviceManager->EndFrame();
    
    // Present to screen
    DeviceManager->Present();
    
    FrameCount++;
}

// 5. Cleanup
DeviceManager->Shutdown();
```

---

## Architecture

### Class Hierarchy

```
FDeviceManager (abstract base class)
└── FDeviceManagerVk (Vulkan implementation)
```

The abstract base class defines common interfaces, while platform-specific implementations handle API details. Only `FDeviceManagerVk` is currently implemented for Vulkan.

### Key Responsibilities

1. **Window Management**: Creates and manages the application window
2. **Device Initialization**: Initializes Vulkan instance, physical device, logical device
3. **Swapchain Management**: Creates, resizes, and destroys the swapchain
4. **Synchronization**: Manages semaphores and frames-in-flight tracking
5. **Framebuffer Management**: Maintains framebuffers matching swapchain images

---

## Core Classes and Structures

### FDeviceCreationParameters

Configuration structure for device and window creation:

```cpp
struct FDeviceCreationParameters
{
    // Window configuration
    bool		  bStartMaximized = false;
    bool		  bStartFullscreen = false;
    bool		  bAllowModeSwitch = false;
    TINT32		  WindowPosX = -1;     // -1 means default placement
    TINT32		  WindowPosY = -1;
    TUINT32		  BackBufferWidth = 1280;
    TUINT32		  BackBufferHeight = 720;
    TUINT32		  BackBufferSampleCount = 1; // Optional HDR MSAA
    TUINT32		  RefreshRate = 0;
    TUINT32		  SwapChainBufferCount = RHI::MAX_FRAMES_IN_FLIGHT;
    nvrhi::Format SwapChainFormat = nvrhi::Format::RGBA8_UNORM;
    TUINT32		  SwapChainSampleCount = 1;
    TUINT32		  VSyncMode = 0;           // 0=FIFO, 1=FifoRelaxed, 2=Immediate
    
    // Feature flags
    bool		  bEnableRayTracingExtensions = false;
    bool		  bEnableComputeQueue = false;
    bool		  bEnableCopyQueue = false;
    
    // Debug and validation
    bool          bEnableDebugRuntime = false;
    bool          bEnableNVRHIValidationLayer = false;
    
    // Adapter selection
    std::wstring	AdapterNameSubstring = L"";
    
    // DPI scaling
    bool          bEnablePerMonitorDPI = false;
    
    // Vulkan-specific (only when HLVM_WINDOW_USE_VULKAN)
    TVector<std::string> RequiredVulkanInstanceExtensions;
    TVector<std::string> RequiredVulkanDeviceExtensions;
    TVector<std::string> RequiredVulkanLayers;
    TVector<std::string> OptionalVulkanInstanceExtensions;
    TVector<std::string> OptionalVulkanDeviceExtensions;
    TVector<std::string> OptionalVulkanLayers;
    TVector<size_t> IgnoredVulkanValidationMessageLocations;
    
    // Features
    bool	bEnableImageFormatD24S8 = true;
    TUINT32 MaxPushConstantSize = 0;
};
```

**Key Parameters:**

- **SwapChainBufferCount**: Number of buffers in swapchain. Default matches `MAX_FRAMES_IN_FLIGHT`. Can be set lower for reduced latency (e.g., `2` for double buffering).
- **VSyncMode**: 
  - `0`: Adaptive/FIFO (tries Mailbox first, falls back to FIFO)
  - `1`: FIFO Relaxed (allows tear)
  - `2`: Immediate (no VSync, may tear)
- **bEnableDebugRuntime**: Enables debug runtime and layers (recommended for development)
- **bEnableNVRHIValidationLayer**: Adds NVRHI validation layer on top of Vulkan layers

---

## Common Workflow

### Phase 1: Initialization

```cpp
// Create DeviceManager instance
auto DeviceManager = FDeviceManager::Create(nvrhi::GraphicsAPI::VULKAN);
HLVM_ENSURE(DeviceManager);

// Modify creation parameters BEFORE calling CreateWindowDeviceAndSwapChain
FDeviceCreationParameters& Params = const_cast<FDeviceCreationParameters&>(DeviceManager->GetDeviceParams());

// Set window dimensions
Params.BackBufferWidth = WIDTH;
Params.BackBufferHeight = HEIGHT;

// Configure swapchain
Params.SwapChainBufferCount = 2; // Double buffer
Params.SwapChainFormat = nvrhi::Format::SRGBA8_UNORM; // sRGB format
Params.VSyncMode = 2; // Disable VSync for testing

// Enable debug features
Params.bEnableDebugRuntime = HLVM_BUILD_DEBUG;
Params.bEnableNVRHIValidationLayer = HLVM_BUILD_DEBUG;

// Add optional extensions for ray tracing (if needed)
Params.bEnableRayTracingExtensions = true;
Params.OptionalVulkanDeviceExtensions = {
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
};

// Create window, device, and swapchain
IWindow::Properties WindowProps;
WindowProps.Title = TEXT("Application");
WindowProps.Extent = {WIDTH, HEIGHT};

bool Success = DeviceManager->CreateWindowDeviceAndSwapChain(WindowProps);
HLVM_ENSURE(Success);
```

### Phase 2: Rendering Loop

```cpp
int FrameIndex = 0;
const int TotalFrames = 100;

while (FrameIndex < TotalFrames)
{
    // Step 1: Begin frame - acquire next swapchain image
    if (!DeviceManager->BeginFrame())
    {
        HLVM_LOG(LogRHI, err, TXT("Failed to begin frame"));
        break;

    }
    
    // Step 2: Get current context
    nvrhi::IDevice* Device = DeviceManager->GetDevice();
    nvrhi::IFramebuffer* CurrentFB = DeviceManager->GetCurrentFramebuffer();
    TUINT32 CurrentBBIndex = DeviceManager->GetCurrentBackBufferIndex();
    
    // Step 3: Record commands
    auto CmdList = Device->createCommandList();
    CmdList->open();
    
    // Clear color attachment
    nvrhi::Color ClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    nvrhi::utils::ClearColorAttachment(CmdList, CurrentFB, 0, ClearColor);
    
    // Set rendering state
    nvrhi::GraphicsState State;
    State.setPipeline(Pipeline)
        .setFramebuffer(CurrentFB)
        .setViewport(Viewport);
    
    CmdList->setGraphicsState(State);
    CmdList->draw(DrawArgs);
    
    CmdList->close();
    Device->executeCommandList(CmdList);
    
    // Step 4: End frame - signal completion
    HLVM_ENSURE(DeviceManager->EndFrame());
    
    // Step 5: Present
    HLVM_ENSURE(DeviceManager->Present());
    
    FrameIndex++;
}
```

### Phase 3: Cleanup

```cpp
// Graceful shutdown
DeviceManager->Shutdown();
DeviceManager.Reset();
```

---

## Resource Access

### Getting Common Resources

```cpp
// Graphics device (for creating textures, shaders, etc.)
nvrhi::IDevice* Device = DeviceManager->GetDevice();

// Renderer information
const char* RendererName = DeviceManager->GetRendererString();
nvrhi::GraphicsAPI API = DeviceManager->GetGraphicsAPI();

// Push constant limit
uint32_t MaxPushConstantSize = DeviceManager->GetMaxPushConstantSize();

// Current framebuffer from active backbuffer
nvrhi::IFramebuffer* CurrFB = DeviceManager->GetCurrentFramebuffer();

// All available backbuffers
TUINT32 BBCount = DeviceManager->GetBackBufferCount();
nvrhi::ITexture* BackBuffer0 = DeviceManager->GetBackBuffer(0);
nvrhi::IFramebuffer* FB0 = DeviceManager->GetFramebuffer(0);
```

### Querying Backend Capabilities

```cpp
// Check enabled Vulkan extensions
if (DeviceManager->IsVulkanInstanceExtensionEnabled(VK_KHR_SURFACE_EXTENSION_NAME))
{
    // Extension is enabled
}

// Get list of enabled extensions
TVector<std::string> InstanceExts, DeviceExts;
DeviceManager->GetEnabledVulkanInstanceExtensions(InstanceExts);
DeviceManager->GetEnabledVulkanDeviceExtensions(DeviceExts);

// Check enabled layers
if (DeviceManager->IsVulkanLayerEnabled("VK_LAYER_KHRONOS_validation"))
{
    // Validation layer is enabled
}
```

---

## Advanced Configuration

### Custom Queue Families

By default, only the graphics queue is requested. To enable compute and copy queues:

```cpp
FDeviceCreationParameters& Params = DeviceManager->GetDeviceParams();
Params.bEnableComputeQueue = true;  // Request dedicated compute queue
Params.bEnableCopyQueue = true;     // Request dedicated transfer queue
```

This will trigger discovery of separate queue families and creation of additional logical device queues if the hardware supports them.

### Ray Tracing Extensions

For ray tracing support:

```cpp
FDeviceCreationParameters& Params = DeviceManager->GetDeviceParams();
Params.bEnableRayTracingExtensions = true;

// Optional: Explicitly list required RT extensions
Params.RequiredVulkanDeviceExtensions = {
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
    VK_KHR_RAY_QUERY_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
};
```

### Multiple Display/DPI Support

```cpp
FDeviceCreationParameters& Params = DeviceManager->GetDeviceParams();
Params.bEnablePerMonitorDPI = true;  // Enable automatic DPI scaling

float ScaleX, ScaleY;
DeviceManager->GetDPIScaleInfo(ScaleX, ScaleY);
// Use these factors to scale UI elements appropriately
```

### Adapter Selection

To select a specific GPU by name:

```cpp
FDeviceCreationParameters& Params = DeviceManager->GetDeviceParams();
Params.AdapterNameSubstring = L"NVIDIA"; // Will prefer NVIDIA GPUs

// The device selector will:
// 1. Filter GPUs matching the substring
// 2. Prefer discrete GPUs over integrated
// 3. Select the first suitable device
```

---

## Swapchain Resizing

Swapchain resizing happens automatically when called explicitly or during resize recovery:

```cpp
// Method 1: Explicit resize request
FUInt2 NewSize = {1920, 1080};
DeviceManager->UpdateWindowSize(NewSize);

// Method 2: Override in your window handler
void MyWindow::OnResize(uint32_t Width, uint32_t Height)
{
    FUInt2 NewSize = {(uint32_t)Width, (uint32_t)Height};
    DeviceManager->UpdateWindowSize(NewSize);
}
```

**Internal Process:**
1. Calls `OnBeforeSwapchainRecreate()` hook (virtually overrideable)
2. Waits for GPU idle
3. Destroys old swapchain and its resources
4. Creates new swapchain with updated dimensions
5. Calls `OnAfterSwapchainRecreate()` hook
6. New framebuffers are automatically created

---

## Synchronization Model

FDeviceManager uses a semaphore-based synchronization model following the Donuts pattern:

### Frame Lifecycle

```plaintext
Frame N                    Frame N+1                  Frame N+2
│                          │                          │
│  Previous Frame Complete │                          │
└─ Signal m_PresentSemaphore_N ──────────────────────►   │     (from EndFrame N)
                                                          │
                   BeginFrame(N+1)                        │
                   ├─ Wait on m_AcquireSemaphore_{N+1}   │
                   └─ Call acquireNextImageKHR(semaphore)
                         │                                 │
                         ▼                                 │
                      Rendering                            │
                   Record GPU Commands                     │
                         │                                 │
                         ▼                                 │
                      EndFrame(N+1)                        │
                   ├─ Signal m_PresentSemaphore_{N+1}      │
                   └─ bCanPresent = true                   │
                         │                                  │
                         ▼         Execute command lists    │
Present waits here ◄── Present(N+1) ◄──────────────────────┼─ Flush semaphores
                   ├─ Wait on m_PresentSemaphore_{N+1}     │
                   └─ presentKHR(swapchain, semaphore)     │
                                                              │
                                              Signal m_PresentSemaphore_{N+2}
                                              (from EndFrame N+2)
```

**Semantic meaning:**
- **BeginFrame()**: Acquires next backbuffer image, waits if GPU needs it
- **Rendering**: Your GPU commands go here
- **EndFrame()**: Signals semaphore that rendering is done
- **Present()**: Queues presentation, submits command lists to present queue

### Frames in Flight

```cpp
// Control maximum in-flight frames (affects latency vs throughput)
FDeviceCreationParameters& Params = DeviceManager->GetDeviceParams();
Params.SwapChainBufferCount = 2;  // Lower = less latency, higher = more parallelism

// Typical values:
// - 2: Low latency, good for interactive applications
// - 3: Balanced approach (default)
// - 4+: Maximum GPU utilization but higher input lag
```

---

## Message Callback System

Debug messages from NVRHI/Vulkan are routed through the message callback:

```cpp
// Get the global callback instance
FNVRHIMessageCallback& Callback = FNVRHIMessageCallback::GetInstance();

// Messages are logged to LogRHI category:
// - Info → HLVM_LOG(LogRHI, info, ...)
// - Warning → HLVM_LOG(LogRHI, warn, ...)
// - Error → HLVM_LOG(LogRHI, err, ...)
// - Fatal → HLVM_LOG(LogRHI, critical, ...)
```

Example output:
```
[LogRHI] INFO: Enabled Vulkan instance extensions:
[LogRHI] INFO:     VK_KHR_surface
[LogRHI] INFO:     VK_KHR_xcb_surface
[LogRHI] INFO: Created Vulkan device: NVIDIA GeForce RTX 3080, API version: 1.3.250
```

---

## Debug Mode Setup

For development builds, debug features are automatically enabled:

```cpp
// These variables control debug mode (defined in DeviceManagerVk.cpp)
// You can also manually set them before CreateWindowDeviceAndSwapChain:

// Global CVar variables (can be toggled at runtime):
g_UseValidationLayers = true;      // Enable Vulkan validation layers
g_UseDebugRuntime = true;          // Enable debug runtime
g_VulkanFastSync = true;           // Use mailbox sync (preferred)
g_vkUsePushConstants = true;       // Enable push constants for renderer
```

**Recommended settings:**
```cpp
// Debug build
bEnableDebugRuntime = true;
bEnableNVRHIValidationLayer = true;
g_UseValidationLayers = !HLVM_BUILD_RELEASE;  // On by default in non-release
g_UseDebugRuntime = HLVM_BUILD_DEBUG;         // On in debug builds
```

---

## Complete Working Example

See `Engine/Source/Runtime/Test/TestDeviceManagerVk.cpp` for a complete integration test with triangle rendering. This includes:

- Full initialization sequence
- Shader loading and pipeline creation
- Vertex/index buffer setup
- Complete render loop with clearing and drawing
- Resource cleanup

**Key excerpt from the test:**

```cpp
#define RECORD_BOOL(test_DeviceManagerVk_Integration) \
{\
\ttry\
\t{\
\t\t// Create DeviceManager\n\t\tauto DeviceManager = FDeviceManager::Create(nvrhi::GraphicsAPI::VULKAN);\n\t\tif (!DeviceManager)\n\t\t\tthrow std::runtime_error("Failed to create device manager");\n\n\t\t// Configure parameters\n\t\tFDeviceCreationParameters& Params = const_cast<FDeviceCreationParameters&>(DeviceManager->GetDeviceParams());\n\t\tParams.BackBufferWidth = WIDTH;\n\t\tParams.BackBufferHeight = HEIGHT;\n\t\tParams.SwapChainBufferCount = 2;\n\t\tParams.bEnableDebugRuntime = HLVM_BUILD_DEBUG;\n\t\tParams.bEnableNVRHIValidationLayer = HLVM_BUILD_DEBUG;\n\n\t\t// Create window+device\n\t\tIWindow::Properties Props;\n\t\tProps.Title = WINDOW_TITLE;\n\t\tProps.Extent = {WIDTH, HEIGHT};\n\t\tProps.Resizable = true;\n\t\tProps.VSync = IWindow::EVsync::Off;\n\n\t\tif (!DeviceManager->CreateWindowDeviceAndSwapChain(Props))\n\t\t\tthrow std::runtime_error("Failed to create device");\n\n\t\t// Render loop\n\t\tconst auto NvrhiDevice = DeviceManager->GetDevice();\n\t\tint Frames = 0;\n\t\twhile (Frames++ < 120)\n\t\t{\n\t\t\tDeviceManager->BeginFrame();\n\t\t\t\n\t\t\tauto CmdList = NvrhiDevice->createCommandList();\n\t\t\tCmdList->open();\n\t\t\tnvrhi::utils::ClearColorAttachment(\n\t\t\t\tCmdList, \n\t\t\t\tDeviceManager->GetCurrentFramebuffer(), \n\t\t\t\t0, \n\t\t\t\tnvrhi::Color(0.1f, 0.1f, 0.1f, 1.0f));\n\t\t\tCmdList->close();\n\t\t\tNvrhiDevice->executeCommandList(CmdList);\n\n\t\t\tDeviceManager->EndFrame();\n\t\t\tDeviceManager->Present();\n\t\t}\n\n\t\tDeviceManager->Shutdown();\n\t\treturn true;\n\t}\n\tcatch (const std::exception& e)\n\t{\n\t\tHLVM_LOG(LogTest, critical, TXT("{}"), FString(e.what()));\n\t\treturn false;\n\t}\n}\n```\

---

## Reference Documentation Sources

As specified in AI_VKRHI4.3.md Rule #5-#10:

1. **NVRHI API Reference**: `/home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Vibe_Coding/NVRHI_API/v2/DOC_api_reference.md`
2. **NVRHI Headers**: `/home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Build/Debug/_deps/nvrhi-src/include/nvrhi/nvrhi.h`
3. **NVRHI Tutorial**: `/home/hangyu5/Documents/Gitrepo-Other/RHI/RBDOOM-3-BFG/neo/extern/nvrhi/doc/Tutorial.md`
4. **Donuts DeviceManager Pattern**: Source references in Rules section

---

## Version History

| Date | Author | Changes |
|------|--------|---------|
| 2025 | YuHang | Initial implementation based on RBDOOM-3-BFG and Donuts |
| 2026 | YuHang | Updated for C++20, added comprehensive documentation |

---

## License

MIT License - See project ROOT for details
