# NVRHI API Fixes Required

## Root Cause Analysis
My PipelineState implementation used incorrect NVRHI API calls. The actual NVRHI API is different:

### What I Got Wrong:
1. ❌ `nvrhi::PrimitiveTopology` - Doesn't exist, should be `nvrhi::PrimitiveType`
2. ❌ `nvrhi::InputRate` - Doesn't exist
3. ❌ `Desc.setPrimitiveTopology()` - Should be `Desc.setPrimType()`
4. ❌ `Desc.setInputSlots()` - Doesn't exist
5. ❌ `Desc.setInputAttributes()` - Doesn't exist  
6. ❌ `Desc.setRaster()` - Should use `Desc.renderState.setRasterState()`
7. ❌ `nvrhi::VertexAttribute` - Should be `nvrhi::VertexAttributeDesc`
8. ❌ `nvrhi::InputSlotDescription` - Doesn't exist
9. ❌ `nvrhi::RenderTargetBlendState` - Should be `nvrhi::BlendState`
10. ❌ `BlendDesc.setRT()` - Wrong API
11. ❌ Direct cast `GraphicsPipelineHandle(PipelineHandle)` - Invalid conversion

### What NVRHI Actually Uses:
✅ `nvrhi::PrimitiveType::TriangleList` (enum at line 2420)
✅ `nvrhi::GraphicsPipelineDesc::setPrimType()` (line 2527)
✅ `nvrhi::GraphicsPipelineDesc::setInputLayout(IInputLayout*)` (line 2529)
✅ `nvrhi::GraphicsPipelineDesc::setRenderState(const RenderState&)` (line 2538)
✅ `nvrhi::RenderState` contains blend/depth/raster states (line 2452)
✅ `nvrhi::VertexAttributeDesc` with `setName()`, `setFormat()`, etc. (line 636)
✅ `Device->createInputLayout()` to create input layout
✅ `Device->createGraphicsPipeline(desc, framebufferInfo)` (line 3653)

## Files That Need Complete Rewrite

### PipelineState.h
**Issues:**
- Line 59: `nvrhi::InputRate` doesn't exist
- Line 221: Invalid cast to GraphicsPipelineHandle
- Line 242: Invalid cast to ComputePipelineHandle
- Line 278: `nvrhi::PrimitiveTopology` doesn't exist

**Fix:** Remove InputRate, use nvrhi::PrimitiveType, remove invalid casts

### PipelineState.cpp  
**Issues:**
- Line 53: `Desc.setPrimitiveTopology()` - wrong method name
- Line 117: `nvrhi::InputSlotDescription` doesn't exist
- Line 120: `Desc.setInputSlots()` doesn't exist
- Line 122: `nvrhi::VertexAttribute` should be `VertexAttributeDesc`
- Line 131: `VertexAttributeSlotClass` doesn't exist
- Line 135: `Desc.setInputAttributes()` doesn't exist
- Line 152: `setDepthBiasSlope` should be `setDepthBiasClamp`
- Line 154: `Desc.setRaster()` doesn't exist
- Line 180: `nvrhi::RenderTargetBlendState` doesn't exist
- Line 189: `BlendDesc.setRT()` doesn't exist

**Fix:** Complete rewrite needed to use correct NVRHI API

## TestRHIObjects.cpp
**Issue:** Line 347 uses `ETextureFormat::RGBA8` but should be `RGBA8_UNORM`

**Fix:** Change to `ETextureFormat::RGBA8_UNORM`

## My Implementations vs NVRHI Reality

### What I Implemented (WRONG):
```cpp
Desc.setPrimitiveTopology(nvrhi::PrimitiveTopology::TriangleList);
Desc.setInputSlots(&SlotDesc, 1);
Desc.setInputAttributes(Attributes.GetData(), Attributes.Num());
Desc.setRaster(RasterDesc);
BlendDesc.setRT(0, RTBlend);
```

### What NVRHI Expects (CORRECT):
```cpp
Desc.setPrimType(nvrhi::PrimitiveType::TriangleList);

// Create input layout first
nvrhi::VertexAttributeDesc Attrs[num];
// ... set attributes ...
InputLayout = device->createInputLayout(Attrs, num);
Desc.setInputLayout(InputLayout);

// Use RenderState
Desc.setRenderState(nvrhi::RenderState()
    .setRasterState(RasterState)
    .setDepthStencilState(DepthStencilState)
    .setBlendState(BlendState));

// Create pipeline with framebuffer info
nvrhi::FramebufferInfo FBInfo = framebuffer->getDesc();
Pipeline = device->createGraphicsPipeline(Desc, FBInfo);
```

## Recommendation

The PipelineState files need a **complete rewrite** to match the actual NVRHI API. The core concepts are correct (builder pattern, state objects) but the actual NVRHI method calls are wrong.

**Files to rewrite:**
1. `PipelineState.h` - Fix types and remove invalid casts
2. `PipelineState.cpp` - Complete rewrite with correct NVRHI API calls
3. `TestRHIObjects.cpp` - Fix format name (RGBA8 → RGBA8_UNORM)

**My other RHI objects are correct:**
✅ Buffer.cpp - Fixed ResourceStates
✅ RenderTarget.h/.cpp - Correct with format conversion
✅ Viewport.h/.cpp - No NVRHI pipeline API usage
