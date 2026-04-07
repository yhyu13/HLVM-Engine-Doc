# Subgoal 3: Implement RenderTarget RHI Objects

## Overview
Implement render target and depth target management for off-screen rendering and swapchain integration.

## Complexity: Medium
Builds on Texture class but adds render target-specific functionality.

## Design

### FRenderTarget
Wrapper around `nvrhi::TextureHandle` configured for color rendering.

**Responsibilities**:
- Texture creation with render target flags
- Clear operations (color, depth)
- Resolve operations (MSAA)
- Transition between states (render target → shader resource)

### FDepthTarget
Wrapper around `nvrhi::TextureHandle` configured for depth/stencil.

**Responsibilities**:
- Depth/stencil texture creation
- Depth clear operations
- Depth state management

### FRenderTargetPool (Optional)
Pool for reusing render targets to avoid frequent allocations.

## File Structure

### Public Headers
1. **RenderTarget.h** (new)
   - `FRenderTarget` - Color render target
   - `FDepthTarget` - Depth/stencil target
   - `ERenderTargetFlags` - Configuration flags
   - `FRenderTargetDesc` - Descriptor structure

### Private Implementations
1. **RenderTarget.cpp** (new)
   - Texture creation with proper flags
   - Clear and resolve operations
   - State transitions

## Implementation Details

### FRenderTarget
```cpp
class FRenderTarget
{
public:
    NOCOPYMOVE(FRenderTarget)
    
    FRenderTarget();
    ~FRenderTarget();
    
    // Initialization
    bool Initialize(
        nvrhi::IDevice* Device,
        uint32_t Width,
        uint32_t Height,
        ETextureFormat Format = ETextureFormat::RGBA8,
        uint32_t SampleCount = 1,
        bool bAllowShaderResource = true);
    
    // Operations
    void Clear(
        nvrhi::ICommandList* CommandList,
        const TFloatColor& Color);
    
    void Resolve(
        nvrhi::ICommandList* CommandList,
        FRenderTarget* DestTarget);
    
    // Access
    [[nodiscard]] nvrhi::TextureHandle GetTextureHandle() const;
    [[nodiscard]] uint32_t GetWidth() const;
    [[nodiscard]] uint32_t GetHeight() const;
    [[nodiscard]] ETextureFormat GetFormat() const;
    [[nodiscard]] uint32_t GetSampleCount() const;
    
    // Debug
    void SetDebugName(const TCHAR* Name);
    
protected:
    nvrhi::TextureHandle TextureHandle;
    nvrhi::IDevice* Device;
    uint32_t Width;
    uint32_t Height;
    ETextureFormat Format;
    uint32_t SampleCount;
    TCharArray<64> DebugName;
};
```

### FDepthTarget
```cpp
class FDepthTarget
{
public:
    NOCOPYMOVE(FDepthTarget)
    
    FDepthTarget();
    ~FDepthTarget();
    
    // Initialization
    bool Initialize(
        nvrhi::IDevice* Device,
        uint32_t Width,
        uint32_t Height,
        ETextureFormat Format = ETextureFormat::D32,
        bool bHasStencil = false);
    
    // Operations
    void Clear(
        nvrhi::ICommandList* CommandList,
        float Depth = 1.0f,
        uint8_t Stencil = 0);
    
    // Access
    [[nodiscard]] nvrhi::TextureHandle GetTextureHandle() const;
    [[nodiscard]] uint32_t GetWidth() const;
    [[nodiscard]] uint32_t GetHeight() const;
    [[nodiscard]] bool HasStencil() const;
    
    // Debug
    void SetDebugName(const TCHAR* Name);
    
protected:
    nvrhi::TextureHandle TextureHandle;
    nvrhi::IDevice* Device;
    uint32_t Width;
    uint32_t Height;
    bool bHasStencil;
    TCharArray<64> DebugName;
};
```

## Key NVRHI APIs
- `nvrhi::TextureDesc`
- `TextureDesc.setIsRenderTarget(true)`
- `TextureDesc.setIsDepthStencil(true)`
- `TextureDesc.setSampleCount()`
- `CommandList->clearTextureFloat()`
- `CommandList->clearDepthStencilTexture()`
- `CommandList->resolveTexture()`

## Integration with Framebuffer
Update `FFramebuffer` to work with new render target classes:
```cpp
// Old
void AddColorAttachment(nvrhi::TextureHandle Texture, ...);

// New (add overloads)
void AddColorAttachment(const FRenderTarget* RenderTarget);
void SetDepthAttachment(const FDepthTarget* DepthTarget);
```

## Success Criteria
- [ ] Can create color render targets with various formats
- [ ] Can create depth targets with/without stencil
- [ ] Clear operations work correctly
- [ ] MSAA resolve works correctly
- [ ] Integration with FFramebuffer works
- [ ] Debug names for GPU debugging
- [ ] No compilation errors

## Dependencies
- Texture class (for base texture handling)
- NVRHI texture API
- Understanding of render target states

## Testing Strategy
- Create render target and clear to solid color
- Create depth target and clear to 1.0
- Test MSAA render target → resolve to non-MSAA
- Verify render target works with framebuffer

## Performance Considerations
- Render target allocation is expensive - pool if possible
- MSAA targets require more memory - use only when needed
- Transitions between states should be minimized
