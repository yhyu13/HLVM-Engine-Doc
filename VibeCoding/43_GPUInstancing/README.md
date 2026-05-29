# GPU Instancing Foundation (Option B: GBuffer + Shadow)

## Status: COMPLETE ✅

## What Was Implemented

### 1. Instanced Vertex Shaders
- **`GBufferInstancedVS.hlsl`** — Reads per-instance model matrices from `StructuredBuffer<float4>` at `t10` using `SV_InstanceID`
- **`ShadowInstancedVS.hlsl`** — Same pattern for shadow map depth rendering
- Both shaders are optional — if not present in the shader data directory, the passes fall back to non-instanced mode with a warning

### 2. FGBufferFillPass — Instanced Path
- Added `FInstancedMeshDrawItem` and `FInstancedRenderDesc`
- Added `RenderInstanced()` method with separate pipeline + binding layout
- Binding layout adds `StructuredBuffer_SRV(10)` for the instance buffer
- Instanced pipeline creation is **optional** — if `GBufferInstancedVS.sblob` is missing, only the non-instanced pipeline is created

### 3. FShadowMapPass — Instanced Path
- Added `FInstancedMeshDrawItem` and `FInstancedRenderDesc`
- Added `RenderInstanced()` method with separate pipeline + binding layout
- Same optional behavior as GBuffer pass

### 4. TestGPUInstancing
- Standalone Vulkan test rendering **100 cubes in a 10×10 grid** with **1 draw call** per pass
- Validates:
  - GBuffer diffuse output has non-black pixels
  - Shadow map has valid depth values (< 1.0)
- Runtime: ~0.86s (windowed, single frame)

### 5. Build System Integration
- Added `create_gpu_instancing_shadermake()` to `ShaderMakeBuild.py`
- Updated `Runtime_cmake.py` to route `TestGPUInstancing` to the new factory

## Key Design Decisions

| Decision | Rationale |
|----------|-----------|
| **StructuredBuffer + SV_InstanceID** vs instanced vertex attributes | Simpler input layout, no extra vertex buffer binding, easier to extend later |
| **Separate pipelines** for instanced vs non-instanced | Existing non-instanced path is completely untouched — zero regression risk |
| **Optional instanced pipeline** | Tests that don't provide instanced shaders continue to work without changes |
| **Instance buffer stores `float4x4`** | 64 bytes per instance, directly indexable by `instanceID * 4` in HLSL |

## Files Changed

### New
- `Engine/Source/Runtime/Test/TestGPUInstancing_Data/GBufferInstancedVS.hlsl`
- `Engine/Source/Runtime/Test/TestGPUInstancing_Data/ShadowInstancedVS.hlsl`
- `Engine/Source/Runtime/Test/TestGPUInstancing_Data/GBufferSponzaPS.hlsl` (copy)
- `Engine/Source/Runtime/Test/TestGPUInstancing_Data/GBufferSponzaVS.hlsl` (copy)
- `Engine/Source/Runtime/Test/TestGPUInstancing_Data/ShadowVS.hlsl` (copy)
- `Engine/Source/Runtime/Test/TestGPUInstancing_Data/ShaderMake.cfg`
- `Engine/Source/Runtime/Test/TestGPUInstancing.cpp`

### Modified
- `Engine/Source/Runtime/Public/Renderer/Deferred/FGBufferFillPass.h`
- `Engine/Source/Runtime/Private/Renderer/Deferred/FGBufferFillPass.cpp`
- `Engine/Source/Runtime/Public/Renderer/Shadow/FShadowMapPass.h`
- `Engine/Source/Runtime/Private/Renderer/Shadow/FShadowMapPass.cpp`
- `Engine/Source/Runtime/ShaderMakeBuild.py`
- `Engine/Source/Runtime/Runtime_cmake.py`

## Deferred to Future Phases

- **Scene integration** (`r_UseInstancing` CVar + mesh grouping in `BuildDrawData()`) — requires scene loader to produce duplicate meshes
- **Instance count threshold** (only instance if ≥ 4) — trivial once grouping is implemented
- **Bindless textures** — needed to instance meshes with different materials; estimated 2–3 days for deferred rasterization

## Validation

```bash
./Build.sh --Config=Debug --Target=TestGPUInstancing --Test        # 1.09s ✅
./Build.sh --Config=Debug --Target=TestSponzaDeferred --Test       # 11.47s ✅
./Build.sh --Config=Debug --Target=TestCameraControls --Test       # 22.25s ✅
./Build.sh --Config=Debug --Target=TestMeshCache --Test            # 9.91s ✅
./Build.sh --Config=Debug --Target=TestRenderGraph --Test          # 0.37s ✅
./Build.sh --Config=Debug --Target=TestRTShadowsGBuffer --Test     # 9.16s ✅
```
