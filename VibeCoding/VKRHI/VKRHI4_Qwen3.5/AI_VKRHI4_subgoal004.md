# Subgoal 4: Implement Viewport with Swapchain Management

## Overview
Implement high-level viewport object that manages swapchain, semaphores, fences, frame buffers, and presentation for windowed rendering.

## Complexity: High
This is the most complex RHI object as it integrates:
- Vulkan swapchain management
- NVRHI device integration
- Synchronization (semaphores, fences)
- Frame buffering (triple buffering)
- Window resize handling

## Design

### FViewport
Manages a swapchain and associated resources for a window.

**Responsibilities**:
- Swapchain creation and recreation
- Framebuffer management for swapchain images
- Synchronization objects (semaphores, fences)
- Frame indexing and triple buffering
- Present operations
- Resize handling

### FViewportManager (Optional)
Manages multiple viewports for multi-window support.

## File Structure

### Public Headers
1. **Viewport.h** (new)
   - `FViewport` - Main viewport class
   - `FViewportDesc` - Viewport configuration
   - `FViewportManager` - Multi-viewport management

### Private Implementations
1. **Viewport.cpp** (new)
   - Swapchain lifecycle management
   - Synchronization setup
   - Present logic
   - Resize handling

## Implementation Details

### FViewport
```cpp
class FViewport
{
public:
    NOCOPYMOVE(FViewport)
    
    FViewport();
    ~FViewport();
    
    // Initialization
    bool Initialize(
        nvrhi::IDevice* Device,
        vk::Instance Instance,
        vk::PhysicalDevice PhysicalDevice,
        vk::Device Device,
        GLFWwindow* Window,
        const FViewportDesc& Desc);
    
    // Frame management
    uint32_t AcquireNextFrame(nvrhi::ICommandList* CommandList);
    void Present(nvrhi::ICommandList* CommandList, uint32_t FrameIndex);
    
    // Access
    [[nodiscard]] nvrhi::FramebufferHandle GetCurrentFramebuffer() const;
    [[nodiscard]] nvrhi::TextureHandle GetCurrentColorTarget() const;
    [[nodiscard]] uint32_t GetWidth() const;
    [[nodiscard]] uint32_t GetHeight() const;
    [[nodiscard]] FFramebuffer* GetFramebuffer(uint32_t Index) const;
    
    // Resize
    void OnWindowResized(int NewWidth, int NewHeight);
    bool IsResizing() const;
    
    // Synchronization
    vk::Semaphore GetRenderFinishedSemaphore(uint32_t FrameIndex) const;
    vk::Fence GetInFlightFence(uint32_t FrameIndex) const;
    
protected:
    // Vulkan objects
    vk::SurfaceKHR Surface;
    vk::SwapchainKHR Swapchain;
    vk::Format SwapchainFormat;
    vk::Extent2D SwapchainExtent;
    std::vector<vk::Image> SwapchainImages;
    std::vector<vk::UniqueImageView> SwapchainImageViews;
    
    // NVRHI objects
    nvrhi::IDevice* Device;
    std::vector<nvrhi::TextureHandle> SwapchainTextures;
    std::vector<TUniquePtr<FFramebuffer>> Framebuffers;
    
    // Synchronization
    std::vector<vk::UniqueSemaphore> ImageAvailableSemaphores;
    std::vector<vk::UniqueSemaphore> RenderFinishedSemaphores;
    std::vector<vk::UniqueFence> InFlightFences;
    std::vector<vk::Fence> ImagesInFlight;
    
    // State
    uint32_t CurrentFrame;
    uint32_t Width;
    uint32_t Height;
    bool bResizing;
    
    TCharArray<64> DebugName;
};
```

### FViewportDesc
```cpp
struct FViewportDesc
{
    uint32_t Width = 800;
    uint32_t Height = 600;
    bool bEnableVSync = true;
    bool bEnableValidation = false;
    uint32_t MaxFramesInFlight = 3;  // Triple buffering
    const TCHAR* DebugName = nullptr;
};
```

## Key Vulkan/NVRHI APIs

### Vulkan
- `vkCreateSwapchainKHR`
- `vkAcquireNextImageKHR`
- `vkQueuePresentKHR`
- `vkCreateSemaphore`
- `vkCreateFence`
- `vkWaitForFences`
- `vkWaitForFences`

### NVRHI
- `nvrhi::vulkan::createDevice()`
- `Device->createTextureFromVulkan()`
- `Device->executeCommandList()`

## Lifecycle Management

### Initialization
1. Create Vulkan surface from GLFW window
2. Create swapchain with desired parameters
3. Get swapchain images
4. Create image views for each swapchain image
5. Wrap swapchain images in NVRHI textures
6. Create framebuffers for each swapchain image
7. Create semaphores and fences

### Per-Frame Rendering
1. Wait for previous frame's fence
2. Acquire next swapchain image
3. Begin render pass on framebuffer
4. Execute rendering commands
5. End render pass
6. Submit command list with wait/signal semaphores
7. Present swapchain image

### Shutdown
1. Wait for GPU to finish (device wait idle)
2. Destroy framebuffers
3. Destroy swapchain textures
4. Destroy swapchain image views
5. Destroy swapchain
6. Destroy semaphores and fences
7. Destroy surface

## Resize Handling
```cpp
void FViewport::OnWindowResized(int NewWidth, int NewHeight)
{
    bResizing = true;
    
    // Wait for GPU to finish
    Device->waitForFences(...);
    
    // Destroy old swapchain resources
    DestroySwapchainResources();
    
    // Recreate swapchain
    CreateSwapchain(NewWidth, NewHeight);
    
    // Recreate dependent resources
    CreateSwapchainImageViews();
    CreateSwapchainTextures();
    CreateFramebuffers();
    
    bResizing = false;
}
```

## Success Criteria
- [ ] Can initialize viewport with window
- [ ] Can acquire and present frames
- [ ] Triple buffering works correctly
- [ ] Window resize is handled gracefully
- [ ] Synchronization prevents GPU/CPU hazards
- [ ] No validation layer errors
- [ ] Clean shutdown with no resource leaks

## Dependencies
- GLFW window management
- Vulkan swapchain API
- NVRHI Vulkan interop
- FFramebuffer class
- Synchronization primitives

## Testing Strategy
- Create viewport and render clear color
- Verify vsync works (frame pacing)
- Test window resize during rendering
- Verify no validation errors
- Test clean shutdown

## Performance Considerations
- Triple buffering for smooth presentation
- Minimize swapchain recreations (throttle resize events)
- Reuse synchronization objects
- Efficient semaphore/fence signaling

## Integration with TestRHIObjects.cpp
Update existing test to use FViewport:
```cpp
// Old: Manual swapchain management
CreateSwapchain(Ctx);
CreateImageViews(Ctx);
CreateSyncObjects(Ctx);

// New: Viewport abstraction
FViewportDesc ViewportDesc;
ViewportDesc.Width = WIDTH;
ViewportDesc.Height = HEIGHT;
Ctx.Viewport = MakeUnique<FViewport>();
Ctx.Viewport->Initialize(..., ViewportDesc);
```
