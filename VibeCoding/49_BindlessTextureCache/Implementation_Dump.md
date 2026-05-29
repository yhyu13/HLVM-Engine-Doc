# PHASE 2 Implementation Dump: Bindless Texture Cache Integration

**Date**: 2026-05-28
**Phase**: 2A (Minimal Bindless Integration)
**Goal**: Extend FTextureCache with optional bindless texture support

---

## Status: COMPLETE ✓

### Files Created/Modified

| File | Action | Status |
|------|--------|--------|
| `Engine/Source/Runtime/Public/Renderer/DescriptorTableManager.h` | Created | ✓ |
| `Engine/Source/Runtime/Private/Renderer/DescriptorTableManager.cpp` | Created | ✓ |
| `Engine/Source/Runtime/Public/Renderer/Texture/TextureCache.h` | Modified | ✓ |
| `Engine/Source/Runtime/Private/Renderer/Texture/TextureCache.cpp` | Modified | ✓ |
| `Engine/Source/Runtime/CMakeLists.txt` | Modified | ✓ |

---

## Implementation Details

### 1. FDescriptorTableManager (Core Infrastructure)

**Purpose**: RAII-managed bindless descriptor table for efficient GPU resource access.

**Key Features**:
- **Hash-based deduplication**: Same resource gets same slot (ignores binding slot in hash)
- **Auto-resizing**: Table doubles when capacity exhausted (minimum 64 slots)
- **Thread-safe**: All public methods use `std::mutex`
- **RAII handles**: `FDescriptorHandle` auto-releases on destruction

**Interface**:
```cpp
class FDescriptorTableManager : public std::enable_shared_from_this<FDescriptorTableManager> {
public:
    // Create with a bindless layout
    FDescriptorTableManager(nvrhi::IDevice* Device, nvrhi::IBindingLayout* Layout);

    // Allocate descriptor and get RAII handle
    FDescriptorHandle CreateDescriptorHandle(nvrhi::BindingSetItem Item);

    // Get/Query
    nvrhi::IDescriptorTable* GetDescriptorTable() const;
    FBindlessIndex GetBindlessIndex(const FPath& FilePath) const;
    uint32_t GetAllocatedCount() const;

    // Helper to create standard texture bindless layout
    static nvrhi::BindingLayoutHandle CreateTextureBindlessLayout(
        nvrhi::IDevice* Device, uint32_t MaxCapacity = 16384);
};
```

**FDescriptorHandle**:
```cpp
class FDescriptorHandle {
public:
    using FDescriptorIndex = int;

    // RAII - auto-releases on destruction
    ~FDescriptorHandle();

    // Get table-relative index
    FDescriptorIndex Get() const;

    // Get heap index (volatile if table resizes)
    FDescriptorIndex GetHeapIndex() const;

    bool IsValid() const;
    void Reset();
};
```

### 2. FTextureCache Bindless Extension

**New Methods**:
```cpp
// Set the descriptor table manager for bindless support
void SetDescriptorTableManager(FDescriptorTableManager* Manager);

// Get bindless index for a texture (-1 if not found or disabled)
FBindlessIndex GetBindlessIndex(const FPath& FilePath) const;

// Check if bindless mode is enabled
bool IsBindlessEnabled() const { return DescriptorTableManager != nullptr; }
```

**Modified Methods**:
- `Insert()`: Now allocates bindless slot if DescriptorTableManager is set
- `Invalidate()`: Releases bindless slot (via FDescriptorHandle RAII)
- `DrawUI()`: Shows bindless slot index per texture

**New FEntry Member**:
```cpp
struct FEntry {
    nvrhi::TextureHandle Texture;
    std::time_t LastWriteTime = 0;
    size_t MemoryBytes = 0;
    FBindlessIndex BindlessIndex = -1;  // NEW: -1 means not allocated
};
```

---

## Integration Pattern

To enable bindless textures:

```cpp
// 1. Create bindless layout
auto BindlessLayout = FDescriptorTableManager::CreateTextureBindlessLayout(Device, 16384);

// 2. Create descriptor table manager
auto DescriptorTable = std::make_shared<FDescriptorTableManager>(Device, BindlessLayout);

// 3. Set it on texture cache
TextureCache.SetDescriptorTableManager(DescriptorTable.get());

// 4. Insert textures - they'll automatically get bindless slots
TextureCache.Insert(Path, Texture);

// 5. Query bindless index when binding
int BindlessIdx = TextureCache.GetBindlessIndex(Path);
if (BindlessIdx >= 0) {
    // Use bindless binding: descriptorTableHeap[BindlessIdx]
}
```

---

## Test Results

```
TestTextureCache: PASSED
TestSponzaDeferred: PASSED
TestRTShadowsGBuffer: PASSED
All 51 tests: PASSED
```

---

## What's Working

1. ✓ FDescriptorTableManager compiles and runs
2. ✓ Thread-safe allocation/deallocation with mutex
3. ✓ Hash-based deduplication (same texture = same slot)
4. ✓ FTextureCache can use bindless mode or traditional mode
5. ✓ Backward compatible: works when DescriptorTableManager is nullptr
6. ✓ All existing tests pass

---

## What's NOT Implemented (Future Phases)

1. **CVar toggle** (`r_BindlessTextures`) - Currently bindless is opt-in only
2. **GBuffer bindless layout** - Need to create bindless-compatible binding layout
3. **Shader bindless texture access** - Need HLSL `ResourceDescriptorHeap` access
4. **FGBufferFillPass integration** - Need to support bindless path in render pass

---

## Next Steps (Phase 2B)

1. Add `r_BindlessTextures` CVar
2. Update `FGBufferFillPass` to create bindless layout
3. Update `FShadowMapPass` to use bindless layout
4. Add shader support for bindless texture sampling

---

## Files

### Engine/Source/Runtime/Public/Renderer/DescriptorTableManager.h
```cpp
// 169 lines - RAII handle + manager class with mutex protection
```

### Engine/Source/Runtime/Private/Renderer/DescriptorTableManager.cpp
```cpp
// 236 lines - Full implementation with thread-safe operations
```

### Engine/Source/Runtime/Public/Renderer/Texture/TextureCache.h
```cpp
// Modified: Added FBindlessIndex, SetDescriptorTableManager(), GetBindlessIndex(), IsBindlessEnabled()
// Modified: Added DescriptorTableManager* member and BindlessIndex to FEntry
```

### Engine/Source/Runtime/Private/Renderer/Texture/TextureCache.cpp
```cpp
// Modified: Insert() now allocates bindless slot if manager set
// Modified: DrawUI() shows bindless column
// Modified: GetBindlessIndex() accessor added
// Modified: SetDescriptorTableManager() mutator added
```

---

## Validation Commands

```bash
# Build
./Build.sh --Config=Debug --Target=TestTextureCache

# Run test
ctest -R TestTextureCache --output-on-failure

# All tests
./Build.sh --Config=Debug --Test
```