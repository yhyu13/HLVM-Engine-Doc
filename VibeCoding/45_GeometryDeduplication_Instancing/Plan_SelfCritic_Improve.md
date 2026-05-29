# Phase 45: Geometry Deduplication & Instancing — Plan & Self-Critique

## Date: 2026-05-26
## Context: Phase 44 (GPU Instancing Scene Integration) complete. Infrastructure is ready but Sponza loader creates separate geometry buffers for identical meshes. Donut analysis shows proper instancing requires: (1) deduplicated geometry buffers, (2) per-instance transforms from scene graph.

---

## 1. Problem Statement

### Current Architecture Issues

1. **No geometry deduplication**: Sponza loader creates separate VB/IB for each mesh entry, even identical geometries
2. **No per-mesh transforms in GPU data**: `FGBufferMeshItem` has no model matrix field
3. **Single global model matrix**: `Params.View->ModelMatrix` applies to ALL meshes

### Donut's Approach (Reference)

```
Scene Load:
├── GeometryData[]     — one per UNIQUE geometry (shared VB/IB handles)
└── InstanceData[]    — one per mesh instance (transform + geometry index)

Render:
└── Instanced draw: InstanceData[instance].transform → SV_InstanceID → buffer
```

### What's Missing in HLVM

1. **Geometry deduplication**: Detect identical meshes by path or content hash, reuse same VB/IB
2. **Per-mesh transforms**: Extract from scene graph (FNode), pass to GPU data
3. **Instance buffer**: StructuredBuffer of transforms, indexed by SV_InstanceID
4. **Integration**: Wire into existing `RenderInstanced()` path

---

## 2. Goal

Modify `FSceneGPUData` to:
1. **Deduplicate geometry buffers** — identical meshes share same VB/IB handles
2. **Extract per-mesh transforms** — from scene graph nodes during `BuildDrawData()`
3. **Create instance buffer** — one `float4x4` per mesh instance with transform
4. **Wire into renderer** — pass instance buffer to `RenderInstanced()`

**Target**: Enable `r_UseInstancing=true` with Sponza and see draw call reduction.

---

## 3. Design

### 3.1 New Data Structures

```cpp
// In FSceneGPUData.h

// Deduplicated geometry entry (one per unique geometry)
struct FMeshGeometry
{
    nvrhi::BufferHandle VertexBuffer;
    nvrhi::BufferHandle IndexBuffer;
    uint32_t IndexCount;
    std::shared_ptr<FStaticMesh> Mesh;
    uint32_t ReferenceCount;  // Number of instances using this geometry
};

// Per-instance data (transform + geometry index)
struct FMeshInstance
{
    uint32_t GeometryIndex;     // Index into MeshGeometries[]
    glm::mat4 Transform;        // World transform from scene graph
};

// Updated FDrawData
struct FDrawData
{
    TVector<FShadowMapPass::FMeshDrawItem> ShadowItems;
    TVector<FDeferredFrameRenderer::FGBufferMeshItem> GBufferItems;

    // NEW: Instancing support
    TVector<FMeshGeometry> Geometries;       // Deduplicated geometries
    TVector<FMeshInstance> Instances;        // Per-instance data

    nvrhi::BufferHandle InstanceBuffer;     // GPU buffer with transforms

    glm::vec3 SceneCenter;
    glm::vec3 BBoxMin;
    glm::vec3 BBoxMax;
    float SceneRadius = 0.0f;
};
```

### 3.2 Geometry Deduplication

```cpp
// In FSceneGPUData::Initialize()

// Build geometry map: detect identical meshes by path or content hash
struct FGeometryKey
{
    FPath MeshPath;
    size_t VertexCount;
    size_t IndexCount;
    // Could also hash vertex/index data for content-based deduplication
};

TMap<FGeometryKey, uint32_t> GeometryMap;  // Key → GeometryIndex

for (const auto& Mesh : StaticMeshes)
{
    FGeometryKey Key{Mesh->GetPath(), Mesh->GetVertices().size(), Mesh->GetIndices().size()};

    if (auto it = GeometryMap.find(Key); it != GeometryMap.end())
    {
        // Reuse existing geometry
        uint32_t GeoIndex = it->second;
        MeshGeometries[GeoIndex].ReferenceCount++;
    }
    else
    {
        // Create new geometry
        uint32_t GeoIndex = MeshGeometries.size();
        GeometryMap[Key] = GeoIndex;
        // Create VB/IB for this geometry
        // ...
    }
}
```

### 3.3 Per-Mesh Transform Extraction

The scene graph (`FScene3DNode`) has transforms. We need to traverse the scene and collect transforms for each mesh.

```cpp
// In FSceneGPUData::BuildDrawData()

// Traverse scene graph and collect mesh instances with transforms
void CollectMeshInstances(FScene3DNode* Node, glm::mat4 ParentTransform)
{
    glm::mat4 WorldTransform = ParentTransform * Node->GetLocalToWorldTransform();

    if (auto MeshNode = dynamic_cast<FMeshNode*>(Node))
    {
        // Find this mesh in our geometry list
        uint32_t GeometryIndex = FindGeometryIndex(MeshNode->GetMesh());

        FMeshInstance Instance;
        Instance.GeometryIndex = GeometryIndex;
        Instance.Transform = WorldTransform;
        Instances.push_back(Instance);
    }

    for (auto Child : Node->GetChildren())
    {
        CollectMeshInstances(Child, WorldTransform);
    }
}
```

### 3.4 Instance Buffer Creation

```cpp
// In FSceneGPUData::BuildDrawData()

// Create instance buffer with transforms
nvrhi::BufferDesc InstBuffDesc;
InstBuffDesc.setByteSize(Instances.size() * sizeof(glm::mat4))
    .setStructStride(sizeof(glm::mat4))
    .setInitialState(nvrhi::ResourceStates::ShaderResource)
    .setKeepInitialState(true)
    .debugName = "InstanceBuffer";
InstanceBuffer = Device->createBuffer(InstBuffDesc);

// Write transforms to buffer
TVector<glm::mat4> InstanceMatrices;
for (const auto& Inst : Instances)
{
    InstanceMatrices.push_back(Inst.Transform);
}
CmdList->writeBuffer(InstanceBuffer, InstanceMatrices.data(), InstanceMatrices.size() * sizeof(glm::mat4));
```

### 3.5 FDeferredFrameRenderer Integration

```cpp
// In FDeferredFrameRenderer::Render()

// When r_UseInstancing=true:
if (CVar_r_UseInstancing && SceneDrawData.Instances.size() > 0)
{
    // Group by (GeometryIndex, Material) to create instanced draw items
    auto Groups = GroupInstancesByGeometryAndMaterial(
        SceneDrawData.Instances,
        SceneDrawData.Geometries,
        Params.GBufferMeshes);

    if (!Groups.empty())
    {
        // Update instance buffer (already in SceneDrawData.InstanceBuffer)
        // For each group, create FInstancedMeshDrawItem with:
        //   - VertexBuffer = Geometries[GeometryIndex].VertexBuffer
        //   - IndexBuffer = Geometries[GeometryIndex].IndexBuffer
        //   - InstanceBuffer = SceneDrawData.InstanceBuffer
        //   - InstanceOffset = offset within buffer
        //   - InstanceCount = group size
        //   - Material = from Params.GBufferMeshes

        GBufferPass.RenderInstanced(CmdList, InstDesc);
    }
    else
    {
        // Fall back
        GBufferPass.Render(CmdList, GBufferDesc);
    }
}
```

### 3.6 FSceneGPUData Changes

| File | Changes |
|------|---------|
| `FSceneGPUData.h` | Add `FMeshGeometry`, `FMeshInstance` structs; update `FDrawData` |
| `FSceneGPUData.cpp` | Modify `Initialize()` for deduplication; modify `BuildDrawData()` to extract transforms and create instance buffer |
| `FDeferredFrameRenderer.cpp` | Modify `Render()` to use instance buffer from scene data |

---

## 4. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| Scene graph doesn't expose transforms correctly | Medium | High | Check `FScene3DNode::GetLocalToWorldTransform()` before implementing |
| Deduplication hash collisions | Low | Medium | Use path + content hash, not just path |
| Breaking existing tests | High | High | Keep non-instanced path as fallback; test thoroughly |
| Instance buffer update overhead | Low | Low | Only update when transforms change (static scene) |
| Memory increase from storing transforms | Low | Low | `N meshes × 64 bytes` is trivial |

---

## 5. Test Plan

### 5.1 TestSponzaDeferred
- `r_UseInstancing=false` (default): Same timing ~11s, same output
- `r_UseInstancing=true`: Should see draw call reduction via GPU profiler

### 5.2 TestGPUInstancing
- Should still pass (~1s) — validates instanced pipeline

### 5.3 GPU Profiler Validation
- Compare draw calls with `r_UseInstancing=0` vs `r_UseInstancing=1`
- Should see significant reduction (potentially 10x+ for Sponza)

---

## 5. Self-Critique Checklist

### Correctness
- [ ] Will it compile? Need to add transforms to scene data flow
- [ ] Will transforms be correct? Need to verify scene graph traversal
- [ ] Will instancing work? Existing RenderInstanced() is tested

### Safety
- [ ] Memory safety: Instance buffer allocation bounded by mesh count
- [ ] Thread safety: Scene loading is single-threaded
- [ ] Fallback: Non-instanced path always available

### Architecture
- [ ] Leverages existing subsystems: FScene3DNode transforms, existing RenderInstanced()
- [ ] Follows project conventions: Uses TVector, nvrhi handles, CVar system
- [ ] Minimal changes: Only modifies FSceneGPUData and FDeferredFrameRenderer

### Efficiency
- [ ] No unnecessary allocations: Instance buffer sized exactly to mesh count
- [ ] No redundant operations: Deduplication happens at load time
- [ ] Static scene optimization: Instance buffer created once

### Testing
- [ ] Existing tests pass with instancing disabled
- [ ] New functionality validated with GPU profiler
- [ ] Regression coverage: All 47 tests

---

## 6. Self-Critique Review — Iteration 2

**Reviewer persona**: Senior staff engineer reviewing junior AI agent's plan.
**Verdict**: REVISE — critical issues identified.
**Confidence**: 8/10

### Issues Found

#### 🔴 Critical: Scene Graph Transform Extraction Not Verified

The plan assumes `FScene3DNode::GetLocalToWorldTransform()` exists and works correctly. I have NOT verified:
- Does `FScene3DNode` have this method?
- Does the scene graph actually store per-mesh transforms?
- How does the scene graph relate to `MeshMultiMaterialMap`?

**Fix**: Must verify scene graph API before committing to this design.

#### 🟡 Major: Deduplication Key May Cause False Positives

Using `FGeometryKey{MeshPath, VertexCount, IndexCount}` could cause false positives:
- Two different meshes with same vertex count but different geometry
- Two meshes with same path but modified geometry

**Fix**: Use content hash (hash of vertex positions) in addition to path/count.

#### 🟡 Major: FSceneGPUData Changes Are Invasive

The plan changes `FSceneGPUData` significantly which could break all tests.

**Fix**: Use `TOptional<FInstancingData>` for backward compatibility.

#### 🟡 Major: Instance Buffer Pass-Through Not Designed

The plan says "wire into existing RenderInstanced() path" but doesn't specify:
- How does instance buffer get from FSceneGPUData to FDeferredFrameRenderer?
- Does `FRenderParams` need new fields?

**Fix**: Add `InstanceBuffer` field to `FDrawData` and pass via `FRenderParams`.

---

## 7. REVISED Design

### 7.1 Backward-Compatible FDrawData Changes

```cpp
struct FDrawData
{
    // EXISTING - unchanged for backward compatibility
    TVector<FShadowMapPass::FMeshDrawItem> ShadowItems;
    TVector<FDeferredFrameRenderer::FGBufferMeshItem> GBufferItems;
    glm::vec3 SceneCenter;
    glm::vec3 BBoxMin;
    glm::vec3 BBoxMax;
    float SceneRadius = 0.0f;

    // NEW - only populated when instancing is enabled
    struct FInstancingData
    {
        TVector<FMeshGeometry> Geometries;
        TVector<FMeshInstance> Instances;
        nvrhi::BufferHandle InstanceBuffer;
    };
    TOptional<FInstancingData> Instancing;
};
```

### 7.2 Improved Deduplication Key

```cpp
struct FGeometryKey
{
    FPath MeshPath;           // Primary key
    size_t VertexDataHash;    // Hash of vertex positions (xyz)
    size_t IndexDataHash;     // Hash of indices
};
```

### 7.3 Instance Buffer Pass-Through

```cpp
// In FSceneGPUData
FDrawData::FInstancingData InstData;
InstData.InstanceBuffer = Device->createBuffer(...);
Result.Instancing = InstData;

// In caller (TestSponzaDeferred)
Params.InstanceBuffer = SceneDrawData.Instancing->InstanceBuffer;
Params.InstanceCount = SceneDrawData.Instancing->Instances.size();

// In FDeferredFrameRenderer::Render()
if (CVar_r_UseInstancing && Params.InstanceBuffer)
{
    // Use instanced path
}
```

### 7.4 Implementation Phases

1. **Phase A**: Verify scene graph API (MUST DO FIRST)
2. **Phase B**: Add instancing data structures with backward compatibility
3. **Phase C**: Geometry deduplication
4. **Phase D**: Transform extraction
5. **Phase E**: Wire into renderer

---

## 8. Success Criteria

- [ ] `TestSponzaDeferred` passes with `r_UseInstancing=false` (~11s)
- [ ] `TestSponzaDeferred` passes with `r_UseInstancing=true` (~11s or faster)
- [ ] `TestGPUInstancing` passes (~1s)
- [ ] GPU profiler shows 50%+ draw call reduction with instancing enabled
- [ ] `./Build.sh --Target=Test` builds all targets, 0 warnings

---

## 9. One-Sentence Summary

**Add geometry deduplication to FSceneGPUData (path+content hash), extract per-mesh transforms from scene graph, create instance buffer with TOptional backward-compatibility, and wire into RenderInstanced() — enabling r_UseInstancing=true to reduce Sponza draw calls by 50%+ via proper instance batching.**

---

## 10. CRITICAL FINDING: Loader Bakes Transforms Into Vertices

### What I Found

After reading `Scene3DLoader.cpp`, I discovered a **fundamental blocker**:

```cpp
// Lines 124-125 in Scene3DLoader.cpp
// Transform position by accumulated hierarchy transform
const aiVector3D AIPosition = ChildTr * AIMesh->mVertices[t];
```

**The loader applies transforms to vertex positions during loading!** This means:

1. **Vertices are in WORLD space** - transforms are baked into geometry
2. **No per-mesh transforms available** - transforms were consumed during loading
3. **Cannot extract transforms** - they're not stored separately

### Why This Blocks Instancing

For proper instancing (like Donut), we need:
- Geometry in LOCAL space (not transformed)
- Per-instance transforms stored separately

Current architecture:
- Geometry is in WORLD space (transformed)
- No per-instance transform storage

### Options

#### Option A: Modify Loader (Recommended)
Stop baking transforms. Store geometry in LOCAL space and export transforms separately.

**Changes needed**:
1. Remove transform from vertex processing (lines 124-125, 128-140)
2. Store transforms alongside meshes in `MeshTree` or separate structure
3. Modify `BuildDrawData()` to use stored transforms

**Impact**: All tests that expect world-space vertices need review.

#### Option B: Re-Load Scene (Inefficient)
Keep loader as-is. For instancing, re-load scene to extract transforms before they're baked.

**Changes needed**:
1. Add option to load scene without transform baking
2. Use for instancing path only

**Impact**: Duplicate loading overhead.

#### Option C: Accept Limitation
Current implementation correctly supports what's possible with baked transforms. Instancing with Sponza won't provide benefits until loader is fixed.

**Changes needed**: None for this phase.

---

## 11. REVISED Plan with Critical Finding

### Recommended Path: Option A (Modify Loader)

1. **Phase 1**: Modify loader to NOT bake transforms
   - Store transforms separately (in `MeshTree` or new structure)
   - Keep vertices in local space

2. **Phase 2**: Modify `FSceneGPUData` to use stored transforms
   - Build instance buffer from stored transforms
   - Deduplicate geometry buffers

3. **Phase 3**: Wire into renderer (existing Phase 44 infrastructure)

### Success Criteria (Revised)

- [ ] `Scene3DLoader` exports transforms without baking into vertices
- [ ] `FSceneGPUData` creates instance buffer from stored transforms
- [ ] `TestSponzaDeferred` passes with `r_UseInstancing=true`
- [ ] GPU profiler shows 50%+ draw call reduction