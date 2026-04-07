previous agent work in here. it is not good due to not borrowing Doom3 render backend code, you must borrow and translate into HLVM's code style. Create file Under /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Public/Renderer/RHI

# Rules
 1, Coding Style under DOC_Coding_Style.md
 2, Asking User for options and give recommanded options
 3, Do not write code without user permission

# Goal

Using NVRHI and DeviceManager VK to reproduce /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Test/TestNVRHIVulkanWindow.cpp in a new test file.



# Subgoals

## 1. DeviceManager Implementation

**Source**: /home/hangyu5/Documents/Gitrepo-Other/RHI/RBDOOM-3-BFG/neo/sys/DeviceManager.h and .cpp and /home/hangyu5/Documents/Gitrepo-Other/RHI/RBDOOM-3-BFG/neo/sys/DeviceManager_VK.cpp


### What was implemented in DeviceManagerVk.cpp:

- **Vulkan Instance Creation**: With extension and layer management
- **Debug Messenger**: Validation layer integration with HLVM logging
- **Window Surface**: GLFW Vulkan surface creation via FGLFW3VulkanWindow
- **Physical Device Selection**: GPU enumeration with capability checking
- **Queue Family Discovery**: Graphics, present, compute, and transfer queues
- **Logical Device Creation**: With Vulkan 1.2 features and extensions
- **Swapchain Management**: Creation, recreation, and present mode selection
- **NVRHI Device Integration**: Wrapping Vulkan device with NVRHI abstraction
- **Frame Rendering**: BeginFrame/EndFrame/Present with proper synchronization
- **Resource Tracking**: Swapchain images, semaphores, and event queries

### Key Features:

- **RAII with Vulkan-HPP**: Uses `vk::UniqueHandle` types for automatic cleanup
- **NVRHI Validation Layer**: Optional validation layer for debugging
- **Fast Sync Support**: Configurable present modes (Mailbox, Immediate, FIFO)
- **Push Constants**: Runtime-configurable push constant support
- **DPI Scaling**: Placeholder for future DPI awareness
- **Error Handling**: Comprehensive logging with HLVM_LOG macros

## 1.1. NVRHI Components Analysis

**Source**: /home/hangyu5/Documents/Gitrepo-Other/RHI/RBDOOM-3-BFG/neo/renderer/NVRHI/

The following components exist in RBDOOM-3-BFG but are **NOT** borrowed because:

1. **RenderBackend_NVRHI.cpp**: Contains id Tech 4-specific renderer backend (idRenderBackend, idRenderWorld, etc.) - HLVM uses different architecture
2. **Image_NVRHI.cpp**: Texture management with idImage - HLVM should implement its own FImage class when needed
3. **Framebuffer_NVRHI.cpp**: Framebuffer objects with idTech-specific framebuffers - HLVM uses NVRHI framebuffers directly
4. **RenderProgs_NVRHI.cpp**: Shader program management with idTech shader syntax - HLVM uses raw SPIR-V
5. **BufferObject_NVRHI.cpp**: Buffer management with idTech-specific features - HLVM uses NVRHI buffers directly
6. **RenderDebug_NVRHI.cpp**: Debug drawing utilities - Can be implemented later if needed

# Subgoals

1,
OUr source inspiration for NVRHI and DeviceManager is from /home/hangyu5/Documents/Gitrepo-Other/RHI/RBDOOM-3-BFG/neo/sys/DeviceManager.h and .cpp and /home/hangyu5/Documents/Gitrepo-Other/RHI/RBDOOM-3-BFG/neo/sys/DeviceManager_VK.cpp

We may not fully complete migration code, We need to finish the /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Private/Renderer/DeviceManager.cpp first.

(AI agent need to fill in details)

1.1,
So there are missing components that we need to borrow from /home/hangyu5/Documents/Gitrepo-Other/RHI/RBDOOM-3-BFG/neo/renderer/NVRHI/

Make sure we borrow them completely, but corp things we don't need or don't have in HLVM

(AI agent need to fill in details)

(AI agent need to fill in more subgoals if needed)

# How to
(AI agent need to fill in details)
