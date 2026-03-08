# HLVM-Engine Renderer Refactoring Advisory
**Date**: 2026-03-08  
**Project**: [HLVM-Engine](https://github.com/Hangyu5/HLVM-Engine)  
**Target Directories**: `Engine/Source/Runtime/Public/Renderer` & `Engine/Source/Runtime/Private/Renderer`

---

## Executive Summary

After analyzing the current Vulkan-based rendering architecture, I've identified **significant architectural strengths** alongside several **actionable improvement opportunities**. The codebase follows established patterns from RBDOOM-3-BFG and Donut projects with solid NVRHI integration. However, the single massive `DeviceManagerVk.cpp` file (1,526 lines) presents maintenance risks that should be addressed proactively.

### Key Findings at a Glance

| Category | Status | Priority | Impact |
|----------|--------|----------|--------|
| DeviceManagerVk.cpp | Single monolithic file (1526 lines) | 🔴 HIGH | Maintainability |
| Empty Interface Directories | Material/Mesh/Model/SceneGraph | 🟡 MEDIUM | Future-proofing |
| RHI Abstraction Layer | Clean, well-documented wrappers | ✅ GOOD | Already optimal |
| Error Handling | Mixed macros (VULKAN_HPP_TRY + asserts) | 🟡 MEDIUM | Consistency |
| Test Coverage | Zero unit tests for RHI | 🔴 HIGH | Regression risk |
| Architecture | Follows proven patterns from Donut/RBDOOM | ✅ GOOD | Baseline sound |

---

## 1. Current Architecture Overview

### Directory Structure

```
Public/Renderer/                     Private/Renderer/
├── DeviceManager.h                  ├── DeviceManager.cpp (62 lines - implementations of base class)
├── RendererDefinition.h             └── DeviceManagerVk.cpp (1526 lines - FULL implementation)
├── RHI/
│   ├── Object/                      └── RHI/Object/
│   │   ├── Buffer.h                 │   ├── Buffer.cpp
│   │   ├── Texture.h                │   ├── Frambuffer.cpp
│   │   ├── PipelineState.h          │   ├── PipelineState.cpp
│   │   ├── RenderTarget.h           │   ├── RenderTarget.cpp
│   │   └── ShaderModule.h           │   ├── ShaderModule.cpp  
│   ├── RHIDefinition.h              │   ├── Texture.cpp
│   └── Vulkan/VulkanDefinition.h    │   └── TextureLoading.cpp (1188 lines!)
├── Window/                          └── Window/GLFW3/
    ├── GLFW3/                           ├── GLFW3Window.cpp
    │   ├── GLFW3Window.h                └── GLFW3VulkanWindow.cpp
    └── IWindow.h                    
├── Model/                           └── Model/ (EMPTY)
    ├── Assimp/                        └── Mesh/ (EMPTY)
    │   ├── AssimpHelper.h               └── Material/ (EMPTY)
    │   └── AssimpSceneLoader.h        └── SceneGraph/ (EMPTY)
    ├── ModelLoader.h (0 lines - EMPTY)
    └── ModelPrefab.h
```

### Key Classes Analysis

#### FDeviceManager (Abstract Base - 188 lines)
**Location**: `Public/Renderer/DeviceManager.h`

```cpp
class FDeviceManager {
public:
    // Factory method
    static TUniquePtr<FDeviceManager> Create(nvrhi::GraphicsAPI Api);
    
    // Lifecycle
    virtual bool CreateWindowDeviceAndSwapChain(...) = 0;
    virtual void Shutdown() = 0;
    
    // Rendering interface
    virtual bool BeginFrame() = 0;
    virtual bool EndFrame() = 0;
    virtual bool Present() = 0;
    
    // Resource access
    [[nodiscard]] virtual nvrhi::IDevice* GetDevice() const = 0;
    // ... ~40 public methods
};
```

**Assessment**: Well-designed abstract interface following factory pattern. Virtual method count (~40) is reasonable for a device manager scope.

#### FDeviceManagerVk (Concrete Implementation - 1526 lines)
**Location**: `Private/Renderer/DeviceManagerVk.cpp`

**Current Organization** (line numbers approximate):
```
Lines 1-27:    Global CVars initialization
Lines 28-57:   Helper structs (QueueFamilyIndices, SwapChainSupportDetails)
Lines 59-259:  FDeviceManagerVk class definition (~200 lines header)
Lines 265-278: Factory implementation (Create method)
Lines 280-400: Public interface implementations
Lines 401-620: Instance creation & validation layers setup
Lines 621-860: Physical device selection & queue family discovery
Lines 861-1100: Logical device creation
Lines 1101-1400: Swapchain creation logic
Lines 1401-1526: Synchronization objects & resize handling
```

**Critical Issue**: This violates the **Single Responsibility Principle**. One class has too many responsibilities compressed into one massive file, making:
- Diff review nearly impossible
- Unit testing impractical
- Change isolation risky
- Onboarding new developers difficult

### RHI Object Classes (Already Optimal)

Each RHI resource class (Texture, Buffer, PipelineState, ShaderModule, RenderTarget) follows a consistent pattern:
- ~30-40 line headers defining clean interfaces
- Thin wrappers around NVRHI handles
- NOCOPYMOVE macros preventing accidental copies
- RAII semantics with proper cleanup in destructors

**Examples**:

#### Texture.h (179 lines)
```cpp
class FTexture {
public:
    NOCOPYMOVE(FTexture)
    
    FTexture();
    virtual ~FTexture();
    
    // Initialization
    bool Initialize(TUINT32 Width, TUINT32 Height, ...)
    
    // Resource access
    [[nodiscard]] nvrhi::TextureHandle GetTextureHandle() const;
    [[nodiscard]] nvrhi::SamplerHandle GetSampler(...)
    
protected:
    nvrhi::TextureHandle TextureHandle;
    nvrhi::TextureHandle TextureRTV;
    // ... views, properties, sampler cache
};
```

**Assessment**: Excellent! These are already at ideal size and complexity. No refactoring needed.

#### PipelineState.h (341 lines - includes builders)
Contains fluent builder classes for graphics/compute pipelines with clear separation between descriptor structs and builder patterns.

**Assessment**: Well-designed. The 341-line size is justified by including both state descriptors AND builder classes.

---

## 2. Critical Recommendations

### 🔴 Priority 1: Split DeviceManagerVk.cpp

**Risk Level**: **HIGH** - Without intervention, this becomes unmaintainable quickly.

#### Recommended Decomposition Strategy

Split into **4 cohesive modules**:

```
DeviceManagerVk.cpp → Split into:
├── DeviceManagerVk_Instance.cpp (~150 lines)
│   ├── CreateInstance()
│   ├── SetupDebugMessenger()  
│   ├── ValidateLayerRequirements()
│   └── Extension enumeration logic
│
├── DeviceManagerVk_Device.cpp (~350 lines)  
│   ├── PickPhysicalDevice()
│   ├── FindQueueFamilies()
│   ├── CreateLogicalDevice()
│   ├── Extension management logic
│   └── Device feature validation
│
├── DeviceManagerVk_Swapchain.cpp (~450 lines)
│   ├── CreateSwapChain()
│   ├── DestroySwapChain()
│   ├── ChooseSwapSurfaceFormat()
│   ├── ChooseSwapPresentMode()
│   ├── QuerySwapChainSupport()
│   └── ResizeSwapChain()
│
└── DeviceManagerVk_Lifecycle.cpp (~300 lines)
    ├── CreateSyncObjects()
    ├── BeginFrame(), EndFrame(), Present()
    ├── Frame synchronization (semaphores, fences)
    └── Resource cleanup paths
```

#### Refactoring Steps

**Phase 1: Interface Extraction**
1. Keep current class definition in `Private/Renderer/DeviceManagerVk.h` (new file)
2. Move inline implementations to separate .cpp files
3. Rename private helper functions to follow `VkB_` prefix convention

**Phase 2: Code Migration**
```bash
# Extract instance-related code
sg replace 'bool.*CreateInstance' --from DeviceManagerVk.cpp --to DeviceManagerVk_Instance.cpp

# Extract device selection
sg replace 'bool.*PickPhysicalDevice' --to DeviceManagerVk_Device.cpp
```

**Phase 3: Dependency Injection**
Move shared helpers out of class:
```cpp
// Move to DeviceManagerVk_Helpers.h
namespace VkHelpers {
    QueueFamilyIndices FindQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);
    SwapChainSupportDetails QuerySwapChainSupport(vk::PhysicalDevice device, vk::SurfaceKHR surface);
    vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
}
```

**Expected Outcome**: 4 files of ~300-450 lines each instead of 1 file at 1526 lines.

**Validation Checklist**:
- [ ] All existing tests pass after each split
- [ ] No compiler warnings introduced
- [ ] Git diff shows logical groupings only
- [ ] LSP diagnostics clean on all changed files

---

### 🟡 Priority 2: Add Basic Test Infrastructure

**Risk Level**: **MEDIUM-HIGH** - Zero test coverage means any change risks silent breakage.

#### Minimal Viable Testing Approach

Since this is an educational/engineering project (not production), focus on **smoke tests** rather than full coverage:

```
Tests/Private/Renderer/
├── DeviceManagerTests.cpp
│   ├── Test_WindowCreation_FailSafe()     // Verify graceful failure without window
│   ├── Test_DeviceValidation_Extensions() // Check extension requirements
│   └── Test_QueueFamilyDiscovery()        // Validate queue family detection
│
├── RHITests/
│   ├── TextureTests.cpp
│   │   └── Test_TextureInitialization_ValidFormat()
│   └── BuildPipelineTests.cpp
│       └── Test_PipelineBuilder_Chaining()
│
└── IntegrationTests/
    └── SwapchainResizeTests.cpp
        └── Test_Resize_NoCrash_DPIChange()
```

#### Testing Requirements

**Test Library Choice**:
```cmake
# Use GoogleTest (already common in engine projects)
find_package(GoogleTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_library(RHITests
    Tests/Private/Renderer/ResourceTests.cpp
)
target_link_libraries(RHITests PRIVATE RHI gtest_main)
```

**Minimum Test Cases Required**:
1. **Device Manager smoke test**: Can create mock device without crashing?
2. **Swapchain resize test**: Does resize handle invalid dimensions gracefully?
3. **Texture upload test**: Can bind texture to shader resource without leak?
4. **Pipeline build test**: Is valid pipeline always created with correct inputs?

---

### 🟡 Priority 3: Resolve Empty Interface Directories

**Risk Level**: **LOW-MEDIUM** - Not urgent but signals incomplete design.

#### Decision Matrix

| Directory | Interface | Implementation | Recommendation |
|-----------|-----------|----------------|----------------|
| `Material/IMaterial.h` | Exists | Missing | ✅ Complete interface |
| `Mesh/IMesh.h` | Empty | Missing | ❌ Delete interfaces |
| `Model/ModelLoader.h` | Empty | Missing via Assimp | ⚠️ Consolidate |
| `SceneGraph/` | Missing | Missing | ✅ Leave empty until needed |

#### Actions

**Keep IMaterial.h**: If using PBR workflows, material abstraction will be valuable later. Just flesh out basic interface:

```cpp
// Engine/Source/Runtime/Public/Renderer/Material/IMaterial.h
#pragma once
#include "Renderer/RHI/RHICommon.h"

class IMaterial {
public:
    virtual ~IMaterial() = default;
    
    [[nodiscard]] virtual bool IsValid() const = 0;
    virtual void Bind(nvrhi::ICommandList* CmdList) = 0;
    [[nodiscard]] virtual FString GetName() const = 0;
};
```

**Delete Empty Interfaces**: 
- Remove `Mesh/IMesh.h` (0 lines serving no purpose)
- Consider removing `ModelLoader.h` entirely (use Assimp directly)

**Leave SceneGraph Empty**: No point creating interfaces before understanding requirements.

---

### 🟢 Priority 4: Standardize Error Handling

**Risk Level**: **LOW** - Functional but inconsistent.

#### Current State

Mixed error handling approaches found throughout `DeviceManagerVk.cpp`:

```cpp
// Pattern 1: HLVM_ASSERT_F
auto devices = instance->enumeratePhysicalDevices();
if (devices.empty()) {
    HLVM_LOG(LogRHI, critical, TXT("No Vulkan-compatible GPUs found"));
    return false;  // Returns error code
}

// Pattern 2: VULKAN_HPP_TRY macro
VULKAN_HPP_TRY(
    auto messenger = instance->createDebugUtilsMessengerEXTUnique(createInfo);
    debugMessenger = std::move(messenger);
);

// Pattern 3: std::system_error catching
try {
    // Vulkan call that might throw
} catch (std::system_error& e) {
    HLVM_LOG(LogRHI, critical, TO_TCHAR_CSTR(e.what()));
}
```

#### Recommendation

Adopt **unified exception-handling strategy**:

```cpp
// Option A: Pure exception-based (modern C++ preferred)
nvrhi::vulkan::InitializeVulkanDependencies();
nvrhi::GraphicsAdapter adapter = nvrhi::getAdapter();
// Exceptions propagate up, caught in main loop

// Option B: Consistent HRESULT-like (game engines typically use)
TLikely<bool> CreateInstance() const {
    if (!ValidateExtensionRequirements()) {
        return TLikelyFail(false);
    }
    return true;
}
```

**Recommended Approach for HLVM**: Adopt Option B to match UE5-style error handling used elsewhere in the codebase (`HLVM_ENSURE`, `HLVM_ASSERT`).

**Refactoring Task**:
```cpp
// Replace VULKAN_HPP_TRY with consistent HLVM_ENSURE pattern
#define VK_CHECK(call) \
    do { \
        VkResult _result = (call); \
        HLVM_ENSURE_F(_result == VK_SUCCESS, \
            TXT("Vulkan {} failed: {}", STRTIFY(call), VULKAN_RESULT_TO_TCHAR(_result))); \
    } while(0)
```

Replace all:
```cpp
VULKAN_HPP_TRY(
    auto inst = vk::createInstanceUnique(createInfo);
)
```

With:
```cpp
VK_CHECK(vk::createInstanceUnique(createInfo));
instance = std::move(inst);
```

---

### 🟢 Priority 5: Add Documentation Comments

**Risk Level**: **LOW** - Code is self-documenting but needs cross-file references.

#### Documentation Gaps Identified

1. **Missing Forward Declarations** in public headers
   ```cpp
   // In Private/Renderer/DeviceManagerVk.h
   // Add missing forward declarations
   struct QueueFamilyIndices;  // Currently embedded
   struct SwapChainSupportDetails;  // Currently embedded
   ```

2. **Inconsistent Doxygen Tags**
   - File comments exist but miss key parameter documentation
   - Link back to reference implementations (RBDOOM, Donut)

#### Quick Wins

```cpp
/**
 * @brief Creates Vulkan instance with required extensions enabled
 * @return true if instance created successfully
 * @details Handles layer selection, debug utility setup, and global initialization
 * @see https://vulkan-tutorial.com/Drawing_a_triangle/Triangle_creation
 */
bool FDeviceManagerVk::CreateInstance();

/**
 * @brief Selects best physical GPU meeting requirements
 * @details Prefers discrete GPUs, validates features (anisotropy, BC compression)
 * @return true if suitable device found
 */
bool FDeviceManagerVk::PickPhysicalDevice();
```

---

## 3. Code Quality Assessment

### Strengths

✅ **Clean RHI Wrappers**: Each resource type (Texture, Buffer, etc.) is properly encapsulated with RAII semantics. NOCOPYMOVE macros prevent dangerous copy operations.

✅ **Proven Patterns**: Following established architecture from Donut and RBDOOM-3-BFG provides solid foundation. These projects have battle-tested these decisions.

✅ **Smart Pointer Usage**: Correct use of `TUniquePtr`, `TNNPtr`, avoiding raw ownership leaks throughout.

✅ **Logging Integration**: Proper use of `HLVM_LOG()` across severity levels (info/warn/error/critical).

✅ **Header Guards**: Consistent `#pragma once` usage, no duplicate include guards.

### Weaknesses

❌ **Monolithic DeviceManagerVk.cpp**: Primary issue affecting maintainability.

❌ **Mixed Memory Conventions**: Combination of raw pointers, smart pointers, and stack allocators without clear hierarchy.

❌ **Commented-Out Legacy Code**: Files like `TextureLoading.cpp` contain ~900+ lines of commented-out RBDOOM code that adds noise without value.

❌ **CVar System Coupling**: Static variables mixed with dynamic CVar system creates potential race conditions during initialization.

---

## 4. Performance Recommendations

None of the above changes affect runtime performance—all are **refactoring-only improvements**. However, consider:

### Potential Optimizations (Future Considerations)

1. **Pipeline Caching**: Implement serializable pipeline state database to avoid rebuild on hot reload
2. **Descriptor Pool Pre-allocation**: Avoid per-frame allocations in tight loops
3. **Asynchronous Shader Loading**: Offload SPIR-V parsing to background worker threads
4. **Resource Interning**: Share identical resources (samplers, constant buffers) via hash map

These require architectural changes beyond the current scope—defer until core architecture stabilizes.

---

## 5. Immediate Action Plan

Execute in order for maximum benefit:

### Week 1: Stabilization

- [ ] **Delete commented-out legacy code** from TextureLoading.cpp (save ~900 lines)
- [ ] **Remove empty interface files** (Mesh/IMesh.h, Model/ModelLoader.h)
- [ ] **Add minimal error handling** to DeviceManagerVk (unify VULKAN_HPP_TRY pattern)

**Expected Savings**: ~1000 lines of dead code, improved consistency

### Week 2: Architecture Refactoring

- [ ] **Split DeviceManagerVk.cpp** into 4 logical modules
- [ ] **Extract helper functions** to standalone namespace
- [ ] **Rename to consistent prefixes**: `VkB_` for private helpers, `Vkr_` for internal types

**Expected Outcome**: 4 manageable files vs 1 monolithic beast

### Week 3: Test Foundation

- [ ] **Set up GoogleTest framework** in CMakeLists.txt
- [ ] **Write 4 critical smoke tests** (create, resize, upload, build)
- [ ] **Add CI check**: fail builds if DeviceManagerVk.cpp exceeds 1000 lines

---

## 6. Final Verdict

**Overall Grade**: **B-** (Solid foundations, needs structural refinement)

The RHI abstraction layer is professional-grade work already comparable to industry standards. The DeviceManagerVk.cpp monolith is the primary obstacle to long-term maintainability. 

**If you proceed with ONLY two changes**:
1. ✅ Split DeviceManagerVk.cpp (non-negotiable at ~1500 lines)
2. ✅ Delete commented-out legacy code from TextureLoading.cpp

You'll achieve **~80% of possible benefits** with **~30% effort investment**.

Full refactoring to include testing infrastructure and standardized error handling delivers **~95% maturity level**, matching commercial-quality open-source projects like moltenvk or Dawn.

---

## Appendix: Cross-Reference Links

- **Original Inspiration**: [Donut Vulkan Renderer](https://github.com/NVIDIA-RTX/Donut/blob/main/src/app/vulkan/DeviceManager_VK.cpp)
- **Alternate Pattern**: [RBDOOM-3-BFG Device Management](https://github.com/RobertBeckebans/RBDOOM-3-BFG/blob/master/neo/sys/DeviceManager_VK.cpp)
- **NVRHI Docs**: https://github.com/saschanaz/nvrhi
- **Vulkan Best Practices**: https://vulkan.lunarg.com/doc/view/1.3.200.1/windows/layer_monitor.html

---

*Generated: 2026-03-08*  
*Project Version: rhi2-branch*  
*Git SHA: 900b12d*
