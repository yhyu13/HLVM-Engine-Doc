# Rules
 1, Coding Style under DOC_Coding_Style.md
 2, Asking User for options and give recommanded options
 3, Do not write code without user permission
 4, If find suppliements not engouth, ask user for more suppliements
# Goal

previous agent Using NVRHI and DeviceManager VK to produce /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Test/TestRHIObjects.cpp, doc at /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Vibe_Coding/VKRHI/VKRHI3_Qwen3.5/, but previous agent did finish all rhi objects, like /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Public/Renderer/RHI/Object/ShaderModule.h, even pipeline, render target, event higher Renderer Object such as viewport (which manage swapchain semaphore fence frame buffer and present),

# Subgoals

each subgoal should dump to a md file for each AI_VKRHI4_subgoalXXX.md

1, Polish /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Public/Renderer/RHI/Object and /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Private/Renderer/RHI/Object, fix any bugs or TODOs

Mainly, vertex and index buffer should split into static and dynamic buffer, and dynamic buffer should be implemented with buffer mapping while static buffer should be implemented with commandlist buffer write.

2, Implement more rhi objects class based on NVRHI, e.g. Viewport, RenderTarget,  ShaderModule, PipelineState, etc.

3, When you are ready, go write a new test under /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Test/
