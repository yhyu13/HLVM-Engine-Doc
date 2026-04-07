# Subgoal 5: Comprehensive Test for All RHI Objects

## Overview
Write a comprehensive test that demonstrates all implemented RHI objects working together to render a triangle with proper pipeline, render targets, and presentation.

## Test Location
`/home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Test/TestRHIObjects_Complete.cpp`

## Test Structure

### Test Goals
1. Initialize Vulkan and NVRHI device
2. Create window and viewport with swapchain
3. Create static vertex and index buffers (triangle)
4. Create shaders (SPIR-V)
5. Create graphics pipeline state
6. Render triangle to swapchain
7. Present frames
8. Clean shutdown

### Test Components

#### 1. Shader Creation
Create minimal SPIR-V shaders for testing:
- **Vertex Shader**: Pass through position and color
- **Fragment Shader**: Output interpolated color

```glsl
// Vertex Shader (vertex.spv)
#version 450
layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Color;
layout(location = 0) out vec3 FragColor;
void main() {
    gl_Position = vec4(Position, 1.0);
    FragColor = Color;
}

// Fragment Shader (fragment.spv)
#version 450
layout(location = 0) in vec3 FragColor;
layout(location = 0) out vec4 OutColor;
void main() {
    OutColor = vec4(FragColor, 1.0);
}
```

#### 2. Test Context Structure
```cpp
struct FCompleteRHITestContext
{
    // Window
    GLFWwindow* Window = nullptr;
    
    // Vulkan
    vk::Instance Instance;
    vk::PhysicalDevice PhysicalDevice;
    vk::Device Device;
    vk::Queue GraphicsQueue;
    
    // NVRHI
    nvrhi::DeviceHandle NvrhiDevice;
    nvrhi::CommandListHandle CommandList;
    
    // RHI Objects
    TUniquePtr<FViewport> Viewport;
    TUniquePtr<FStaticVertexBuffer> VertexBuffer;
    TUniquePtr<FStaticIndexBuffer> IndexBuffer;
    TUniquePtr<FShaderModule> VertexShader;
    TUniquePtr<FShaderModule> FragmentShader;
    TUniquePtr<FPipelineState> PipelineState;
    
    // State
    bool bInitialized = false;
};
```

#### 3. Test Functions

**InitializeVulkan()**
- Create Vulkan instance
- Create surface from GLFW window
- Select physical device
- Create logical device and queues

**InitializeNVRHI()**
- Create NVRHI Vulkan device
- Create command list

**CreateRHIResources()**
- Create viewport (swapchain, framebuffers, sync objects)
- Create vertex buffer (triangle vertices)
- Create index buffer (3 indices)
- Load shaders from SPIR-V files
- Create graphics pipeline state
  - Set shaders
  - Set vertex input layout
  - Set primitive topology
  - Set rasterizer state
  - Set depth/stencil state (disabled)
  - Set blend state
  - Set framebuffer layout

**RenderFrame(uint32_t FrameIndex)**
- Acquire next frame from viewport
- Open command list
- Begin render pass
  - Set viewport and scissor
  - Bind pipeline
  - Bind vertex/index buffers
  - Draw indexed (3 indices)
- End render pass
- Close command list
- Execute command list
- Present frame

**CleanupRHIResources()**
- Destroy all RHI objects in reverse order
- Wait for GPU to finish
- Destroy Vulkan device and instance

#### 4. Main Test Function
```cpp
RECORD_BOOL(test_RHI_Objects_Complete)
{
    FCompleteRHITestContext Ctx;
    
    try
    {
        // Initialize window
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        Ctx.Window = glfwCreateWindow(800, 600, "Complete RHI Test", nullptr, nullptr);
        
        // Initialize Vulkan
        InitializeVulkan(Ctx);
        
        // Initialize NVRHI
        InitializeNVRHI(Ctx);
        
        // Create RHI resources
        CreateRHIResources(Ctx);
        
        // Render loop
        FTimer Timer;
        while (!glfwWindowShouldClose(Ctx.Window))
        {
            glfwPollEvents();
            
            // Auto-close after 3 seconds
            if (Timer.MarkSec() > 3.0)
            {
                glfwSetWindowShouldClose(Ctx.Window, GLFW_TRUE);
            }
            
            // Render
            uint32_t FrameIndex = Ctx.Viewport->AcquireNextFrame(Ctx.CommandList);
            RenderFrame(Ctx, FrameIndex);
            Ctx.Viewport->Present(Ctx.CommandList, FrameIndex);
        }
        
        // Wait for GPU
        Ctx.Device.waitIdle();
        
        // Cleanup
        CleanupRHIResources(Ctx);
        glfwDestroyWindow(Ctx.Window);
        glfwTerminate();
        
        cout << "Complete RHI test passed!" << endl;
        return true;
    }
    catch (const exception& e)
    {
        cerr << "Fatal Error: " << e.what() << endl;
        CleanupRHIResources(Ctx);
        if (Ctx.Window) glfwDestroyWindow(Ctx.Window);
        glfwTerminate();
        return false;
    }
}
```

## Test Assets Required
Create test shader files in test data directory:
- `Engine/Source/Runtime/Test/Data/Shaders/Triangle.vert.spv`
- `Engine/Source/Runtime/Test/Data/Shaders/Triangle.frag.spv`

Or embed shader bytecode directly in test for simplicity.

## Success Criteria
- [ ] Test compiles without errors
- [ ] Window opens with 800x600 resolution
- [ ] Colored triangle renders correctly
- [ ] No Vulkan validation errors
- [ ] No NVRHI errors
- [ ] Vsync works (no tearing)
- [ ] Clean shutdown with no crashes
- [ ] No resource leaks (verified with validation layers)

## Dependencies
- All previous RHI objects implemented
- GLFW window library
- SPIR-V shaders (embedded or file)
- Vulkan validation layers (debug builds)

## Debugging Tips
- Enable Vulkan validation layers
- Use NVRHI debug callbacks
- Set debug names on all objects
- Check return values of all initialization calls
- Use RenderDoc or Nsight Graphics for GPU debugging

## Performance Considerations
- Triple buffering should maintain 60 FPS with vsync
- CPU-GPU synchronization should be efficient
- No unnecessary pipeline state changes
- No resource allocations in render loop

## Future Extensions
- Add depth testing with depth buffer
- Add multiple render targets (MRT)
- Add uniform buffers for transforms
- Add texture sampling
- Add instancing
- Add compute shader test
