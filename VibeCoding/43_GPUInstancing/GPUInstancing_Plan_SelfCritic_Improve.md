# GPU Instancing Foundation — Plan & Self-Critique

## Date: 2026-05-25
## Context: Deferred renderer is mature (14 passes, render graph, PBR textures). Next bottleneck is draw call count.

---

## 1. Why GPU Instancing Now?

`TestSponzaDeferred` renders ~300 mesh draw calls per frame. Many of these are identical geometry (columns, chains, architectural elements) with identical materials. On modern GPUs, 300 draw calls is trivial, but as scene complexity grows, draw calls become the first wall.

**The engine already has everything needed for instancing**:
- ✅ NVRHI supports `instanceCount` in `DrawArguments`
- ✅ NVRHI input layouts support `isInstanced` vertex attributes
- ✅ `FGBufferFillPass` already batches draws per command list
- ✅ `FSceneGPUData::BuildDrawData()` produces a flat list of draw items

What's missing: A code path that groups identical meshes and draws them with `instanceCount > 1`.

---

## 2. Goal

Add an **instanced draw path** to `FGBufferFillPass` that can render multiple instances of the same mesh with a single `drawIndexed()` call. Validate with a standalone test that renders 100+ cubes in one draw call.

---

## 3. Design

### 3.1 Instance Data Format

Each instance needs a model matrix. For the MVP, we store a `float4x4` per instance in a GPU buffer.

```cpp
struct FInstanceData
{
    float ModelMatrix[16];
};
```

Buffer usage:
- Created as `nvrhi::BufferDesc` with `isVertexBuffer = true`
- Bound to vertex buffer slot 1 (slot 0 = mesh geometry)
- Updated each frame from CPU (scene is static, but camera moves)

### 3.2 Shader Approach: Structured Buffer + SV_InstanceID

Instead of adding instanced vertex attributes (which requires input layout changes and extra vertex buffer binding), we use the **modern approach**:

```hlsl
StructuredBuffer<float4x4> InstanceMatrices : register(t10);

// In vertex shader:
uint instanceID : SV_InstanceID;
float4x4 modelMatrix = InstanceMatrices[instanceID];
```

**Why structured buffer over instanced vertex attributes**:
- Simpler input layout (no extra vertex attributes)
- No extra vertex buffer binding
- Easier to extend later (can add per-instance material indices, colors, etc.)
- Works with the existing vertex shader by just adding the buffer binding

**Shader permutation strategy**:
- Create `GBufferSponzaInstancedVS.hlsl` — same as `GBufferSponzaVS.hlsl` but reads `InstanceMatrices[SV_InstanceID]` instead of `ModelMatrix` from the constant buffer
- Keep existing `GBufferSponzaVS.hlsl` unchanged for non-instanced draws
- `FGBufferFillPass` creates TWO pipelines: one for regular draws, one for instanced draws

### 3.3 FGBufferFillPass Changes

**New structures**:
```cpp
struct FInstancedMeshDrawItem
{
    nvrhi::BufferHandle VertexBuffer;
    nvrhi::BufferHandle IndexBuffer;
    nvrhi::BufferHandle InstanceBuffer;
    uint32_t IndexCount;
    uint32_t InstanceCount;
    FMaterialBinding Material;
};
```

**New API**:
```cpp
void RenderInstanced(nvrhi::ICommandList* CmdList, const FViewConstants& ViewConstants,
                     const FInstancedMeshDrawItem* Items, uint32_t ItemCount);
```

**Pipeline setup**:
- Existing pipeline stays untouched
- New instanced pipeline: same PS, new VS (`GBufferSponzaInstancedVS.hlsl`), same render targets
- New binding layout adds `StructuredBuffer` SRV at register t10

### 3.4 Integration into FDeferredFrameRenderer

`FDeferredFrameRenderer::Render()` currently calls:
```cpp
GBufferPass.Render(CmdList, GBufferDesc);
```

For the MVP, we add an **alternative path**:
```cpp
if (bUseInstancing)
{
    GBufferPass.RenderInstanced(CmdList, ViewConstants, InstancedItems, InstancedItemCount);
}
else
{
    GBufferPass.Render(CmdList, GBufferDesc);
}
```

`bUseInstancing` is controlled by a CVar: `r_UseInstancing` (default false for the MVP, to avoid breaking existing tests).

### 3.5 Scene Data Grouping (Stretch)

`FSceneGPUData::BuildDrawData()` produces `TVector<FGBufferMeshItem>`. For instancing, we group by `(VertexBuffer, IndexBuffer, Material.Textures)`:

```cpp
TMap<FMeshKey, TVector<glm::mat4>> MeshToInstances;
for (const auto& Item : GBufferItems)
{
    FMeshKey Key{Item.VertexBuffer, Item.IndexBuffer, Item.Material.DiffuseTexture};
    MeshToInstances[Key].push_back(Item.ModelMatrix);
}
```

Then create one `FInstancedMeshDrawItem` per unique key, with `InstanceBuffer` containing all matrices.

**Risk**: This changes `BuildDrawData()` which is used by ALL tests. Mitigation: make grouping opt-in via `bUseInstancing` flag. When false, `BuildDrawData()` produces the same output as before.

---

## 4. Test Plan

### 4.1 TestGPUInstancing

A standalone Vulkan test that:
1. Creates a simple cube mesh (8 vertices, 36 indices)
2. Creates 100 instance matrices (grid layout)
3. Calls `FGBufferFillPass::RenderInstanced()`
4. Renders to a small GBuffer
5. Reads back the diffuse texture and verifies non-black pixels
6. Uses GPU profiler to confirm 1 draw call (not 100)

### 4.2 Regression Tests

- `TestSponzaDeferred` with `r_UseInstancing=0` must still pass
- `TestCameraControls` must still pass
- `TestMeshCache` must still pass

---

## 5. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| NVRHI structured buffer + SV_InstanceID doesn't work as expected | Low | High | Test early with `TestGPUInstancing`. If blocked, fall back to instanced vertex attributes. |
| Instanced pipeline breaks existing non-instanced path | Medium | High | Keep two separate pipelines. Non-instanced path is untouched. |
| Instance buffer update every frame is CPU-heavy | Low | Medium | Scene is static. Instance data only changes if camera moves (view matrix, not model matrices). For static scenes, instance buffer is created once at init. |
| `BuildDrawData()` grouping changes break other tests | Medium | High | Make grouping opt-in via `bUseInstancing`. Default is false. |

---

## 6. Self-Critique of This Plan

### Strengths
- **Builds on mature infrastructure**: The GBuffer pass is well-understood. We're adding a parallel code path, not rewriting it.
- **Modern approach**: Structured buffer + `SV_InstanceID` is the standard approach in D3D12/Vulkan. It scales better than per-instance vertex attributes.
- **Testable**: A standalone `TestGPUInstancing` can validate the feature without needing a full scene loader.
- **Backward compatible**: Existing tests use the non-instanced path. The instanced path is opt-in.

### Weaknesses
- **Limited immediate impact**: Sponza's glTF loader may not deduplicate meshes, so `BuildDrawData()` grouping might not find many instances. The real win comes when we have a scene with intentional instancing (foliage, particles).
- **Overhead for small instance counts**: If a mesh only has 2-3 instances, instancing might not be faster than individual draws due to buffer update overhead. We should add a threshold (e.g., only instance if `count >= 4`).
- **Material uniformity constraint**: The MVP only instances meshes with identical materials. This is correct but restrictive. Real engines use bindless textures to instance meshes with different materials.

### Decisions
- **Accept the plan**: Even if Sponza doesn't benefit immediately, the infrastructure is needed for future scenes.
- **Add instance count threshold**: Only use instancing if `InstanceCount >= 4`. Below that, individual draws are fine.
- **Defer bindless textures**: Material variety per instance requires descriptor indexing. That's Month 7+.

---

## 7. Success Criteria

- [ ] `TestGPUInstancing` passes (renders 100 cubes, 1 draw call, non-black output)
- [ ] `TestSponzaDeferred` passes with `r_UseInstancing=0` (~11s)
- [ ] `TestSponzaDeferred` passes with `r_UseInstancing=1` (if scene grouping is implemented)
- [ ] `TestCameraControls` passes
- [ ] `./Build.sh --Target=Test` builds all targets, 0 warnings
- [ ] GPU profiler shows draw call reduction when instancing is enabled

---

## 8. One-Sentence Summary

**Add an instanced draw path to the GBuffer pass using structured buffer instance matrices and SV_InstanceID, validate with a standalone test, and keep the existing non-instanced path untouched for backward compatibility.**
