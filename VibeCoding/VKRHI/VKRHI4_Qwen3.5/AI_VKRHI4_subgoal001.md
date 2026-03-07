# Subgoal 1: Polish Buffer Classes - Static/Dynamic Separation

## Overview
Split vertex and index buffers into separate static and dynamic implementations based on usage patterns.

## Design Decisions

### Static Buffers
- **Use Case**: Geometry that doesn't change (meshes, static objects)
- **Implementation**: 
  - Created with `nvrhi::BufferDesc::setIsVolatile(false)`
  - Data uploaded via `CommandList->writeBuffer()`
  - GPU-only memory (not CPU accessible)
  - Better performance for rendering
- **Classes**: `FStaticVertexBuffer`, `FStaticIndexBuffer`

### Dynamic Buffers
- **Use Case**: Frequently updated data (particle systems, UI, streaming geometry)
- **Implementation**:
  - Created with `nvrhi::BufferDesc::setIsVolatile(true)` and CPU access flags
  - Data uploaded via `Device->mapBuffer()` / `Device->unmapBuffer()`
  - CPU-visible memory (slower GPU access but flexible)
  - Support for orphaning (discard previous contents)
- **Classes**: `FDynamicVertexBuffer`, `FDynamicIndexBuffer`

## File Changes

### Public Headers
1. **Buffer.h** - Refactor existing classes:
   - Keep `FVertexBuffer` and `FIndexBuffer` as base classes (optional)
   - Add `FStaticVertexBuffer : public FVertexBuffer`
   - Add `FDynamicVertexBuffer : public FVertexBuffer`
   - Add `FStaticIndexBuffer : public FIndexBuffer`
   - Add `FDynamicIndexBuffer : public FIndexBuffer`
   - Remove commented TODO code
   - Fix any bugs found

2. **New: BufferStatic.h** (optional, if separation needed)
3. **New: BufferDynamic.h** (optional, if separation needed)

### Private Implementations
1. **Buffer.cpp** - Implement static/dynamic variants:
   - Static: Use `writeBuffer()` with proper state transitions
   - Dynamic: Use `mapBuffer()` with `CpuAccessMode::Write`
   - Add proper error handling with `HLVM_ENSURE_F`
   - Set debug names for GPU debugging

## Implementation Details

### FStaticVertexBuffer
```cpp
class FStaticVertexBuffer : public FVertexBuffer
{
public:
    bool Initialize(
        nvrhi::ICommandList* CommandList,
        nvrhi::IDevice* Device,
        const void* VertexData,
        size_t VertexDataSize);
    
    // No Update() method - static buffers shouldn't be updated
    // Users should use FDynamicVertexBuffer for updates
};
```

### FDynamicVertexBuffer
```cpp
class FDynamicVertexBuffer : public FVertexBuffer
{
public:
    bool Initialize(
        nvrhi::IDevice* Device,
        size_t BufferSize,
        bool UseOrphaning = true);  // Discard previous buffer on update
    
    void* Map(nvrhi::CpuAccessMode AccessMode = nvrhi::CpuAccessMode::Write);
    void Unmap();
    
    void Update(
        nvrhi::ICommandList* CommandList,
        const void* Data,
        size_t DataSize,
        size_t DstOffset = 0,
        bool DiscardPrevious = false);  // Orphaning support
};
```

## Success Criteria
- [ ] All TODO comments in Buffer.h/cpp resolved
- [ ] Static buffer uses writeBuffer() only
- [ ] Dynamic buffer uses map/unmap with proper flags
- [ ] No type errors or warnings
- [ ] Debug names work correctly
- [ ] Existing TestRHIObjects.cpp still compiles (may need updates)

## Dependencies
- NVRHI buffer API understanding
- Existing Buffer.h/cpp structure
- HLVM coding style (DOC_Coding_Style.md)

## Risks
- Breaking existing code that uses FVertexBuffer
- May need to update TestRHIObjects.cpp to use new classes
- Ensure proper memory barriers for static buffer uploads
