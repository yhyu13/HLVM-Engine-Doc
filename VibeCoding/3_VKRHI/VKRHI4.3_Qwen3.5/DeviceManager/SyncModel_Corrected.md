# FDeviceManager - Synchronization Model Documentation

## Key Correction from Previous Version

**My previous diagram was WRONG.** The correct semantic flow is:

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

## Correct Semantic Meaning

| Method | Actual Action | Semaphore Operation | What It Waits For |
|--------|---------------|---------------------|-------------------|
| **BeginFrame()** | Acquire next swapchain image | `wait()` on **Acquire Semaphore** | GPU signals it's done with this backbuffer |
| **Rendering** | Record your GPU commands | - | Runs parallel with presentation of older frames |
| **EndFrame()** | Signal rendering complete | `signal()` **Present Semaphore** | Nothing - just notifies Present can proceed |
| **Present()** | Queue display on screen | `wait()` on **Present Semaphore** | GPU finished rendering (signaled by EndFrame) |

## Implementation Details (DeviceManagerVk.cpp)

### BeginFrame - Line 1324
```cpp
// Wait for previous frame before acquiring backbuffer
m_NvrhiDevice->queueWaitForSemaphore(nvrhi::CommandQueue::Graphics, semaphore, 0);
```
- Uses a pool of acquire semaphores (round-robin indexing)
- Waits on acquire semaphore BEFORE calling `acquireNextImageKHR`
- This ensures we don't steal a backbuffer from an incomplete previous frame

### EndFrame - Line 1344
```cpp
// Signal that rendering is complete
const auto& presentSemaphore = m_PresentSemaphores[m_SwapChainIndex];
m_NvrhiDevice->queueSignalSemaphore(nvrhi::CommandQueue::Graphics, presentSemaphore, 0);
bCanPresent = true;
```
- Signals the corresponding present semaphore  
- Sets `bCanPresent = true` to allow Present() to proceed
- This is a non-blocking signal (added to GPU command queue)

### Present - Lines 1370-1381
```cpp
// First execute all buffered commands (flushes semaphore signals)
m_NvrhiDevice->executeCommandLists(nullptr, 0);

// Build present info with wait semaphore
const auto& presentSemaphore = m_PresentSemaphores[m_SwapChainIndex];
vk::PresentInfoKHR presentInfo;
presentInfo.setWaitSemaphoreCount(1)
           .setPWaitSemaphores(&presentSemaphore)
           // ... rest of setup
presentQueue.presentKHR(presentInfo);
```
- **Critical**: Must execute command lists BEFORE presentKHR to ensure semaphore signals are actually processed
- Then waits on present semaphore via presentKHR
- Only then does presentation occur

## Why This Order Matters

1. **BeginFrame waits first**: Ensures we don't reuse a backbuffer while GPU is still writing to it
2. **EndFrame signals immediately**: Tells "someone else can use this image now" (the Present phase)
3. **Present executes then waits**: Flushes any pending GPU commands (semaphore signals need execution), THEN blocks waiting for those computations to finish
   
This implements proper 3-frame pipelining with double-buffered acquire semaphores and single present semaphore per swapchain image.

## Reference Source Code

See implementation in:
- `/home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Private/Renderer/DeviceManagerVk.cpp` lines 1273-1438
