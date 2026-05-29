# CRITIC REVIEW — Iteration 1

**Reviewer persona**: Senior staff engineer reviewing a junior AI agent's roadmap.
**Verdict**: REVISE — multiple critical issues, scope bloat, and missing broken things.
**Confidence**: 8/10

---

## Concerns Addressed in Revision

### 🔴 Critical: Ignored ACTUALLY BROKEN Build Scripts
**Original sin**: The brainstorm put "shader hot-reload" and "GPU profiler" as P0 but completely ignored the build system which has **real bugs**:
- `AGENTS.md` says `./Build.sh --Test` but **no root `Build.sh` exists** — users must know to run `Engine/Source/Runtime/Build.sh`
- `Common/Build.sh` has **string concat bugs**: `cbuild_param+="--verbose"` produces `-j 8--verbose`
- `Common/Build.sh` has **self-reference bug**: `cbuild_param+="--target ${BuildTarget} ${cbuild_param}"`
- `Runtime/Build.sh` **TestRepeatNum parse bug**: checks `--TestRepeatNum` instead of `--TestRepeatNum=*`, so value is never extracted
- Runtime and Common `Build.sh` are **90% copy-pasted**

**Verdict**: Build bugs waste dev time every single day. They outrank "nice to have" rendering features.

### 🔴 Critical: Suggested Features With Missing Prerequisites
**Original sin**: Proposed ECS, Animation, Physics, Editor as mid/long-term without acknowledging:
- **No serialization system** — ECS without save/load is useless
- **SkeletalMesh header exists but NO skinning GPU pipeline** — animation is a facade
- **FAssetLoader and FScene3DLoader are NOT integrated** — resources have no unified lifetime
- **No GPU instancing** — scene graph is CPU-only transform hierarchy

**Verdict**: Cut all features that don't have prerequisite infrastructure. Build the prerequisites first.

### 🟡 Major: "Shader Hot-Reload" — Didn't Check Existing Infrastructure
**Original sin**: Proposed building file watcher + pipeline recreation from scratch. But explore agent found:
> "no hot-reload outside of `FAssetLoader` file watching"

This implies `FAssetLoader` **already has file watching**. Hot-reload should leverage it, not reinvent it.

**Verdict**: Investigate `FAssetLoader`'s watcher first. Scope changed to "wire existing watcher to shader `.sblob` files."

### 🟡 Major: "Bindless Textures" — Not Verified
**Original sin**: Proposed bindless without checking if current NVRHI fork supports `VK_EXT_descriptor_indexing`. The explore agent noted "no bindless resources" but didn't confirm capability. Adding bindless requires shader changes, descriptor array management, and fallback paths.

**Verdict**: DEFER until NVRHI capability is verified. Not a current pain point (8 SRVs is plenty for test scenes).

### 🟡 Major: "Render Graph" — Massive Scope Underestimated
**Original sin**: Proposed minimal render graph in 2-3 weeks. But `FDeferredFrameRenderer` has **15+ manually wired passes** with complex interdependencies (GBuffer → Shadow → SSAO → Blur → Contact Shadows → Lighting → SSR → TAA → Motion Blur → DOF → Bloom → Exposure → Tone Map → Lens Effects). Even a "minimal" graph needs resource aliasing, barrier inference, and transient memory allocation.

**Verdict**: DEFER. Manual wiring is annoying but functional. The real blocker is **ResourceManager** — without it, a render graph has no assets to reference.

### 🟢 Minor: Good Call on Clustered Shading / GI / VSM
**Original**: Correctly deferred clustered shading, GI, VSM.
**Verdict**: [ACK] Keep deferred.

---

## Concerns Rejected (Marked [ACK])

- **"GPU Profiler is P0"** → Downgraded to P1. Validation + build fixes are true P0.
- **"Texture streaming is P2"** → Downgraded to P2 MVP only (load lowest mip first). Full streaming needs ResourceManager.
- **"ImGui CVar browser is P1"** → Kept as P1. Trivial effort, high artist value.

---

# REVISED PLAN — HLVM-Engine Roadmap

## Philosophy

**Build prerequisites before features. Fix broken things before adding new things. Cut anything that requires infrastructure that doesn't exist yet.**

The engine is a **renderer testbed with excellent post-process**. Don't pretend it's a game engine yet. Build the bridge from "testbed" to "engine" methodically.

---

## P0 — BROKEN THINGS (Fix This Week)

### P0.1 Fix Build System Bugs
**Pain**: Every dev interaction with the build is friction.

| Bug | Fix |
|-----|-----|
| No root `Build.sh` | Create `./Build.sh` that delegates to `Engine/Source/Runtime/Build.sh` or `Common/Build.sh` based on target |
| String concat bugs in `Common/Build.sh` | Fix missing spaces: `cbuild_param+=" --verbose"` |
| Self-reference bug in `Common/Build.sh` | Remove `${cbuild_param}` from target append |
| `TestRepeatNum` parse bug | Fix pattern match to `--TestRepeatNum=*` |
| 90% copy-paste between Runtime/Common `Build.sh` | Extract common functions to `BuildCommon.sh` |

**Effort**: Low (1-2 days)
**Impact**: Massive — every build, every test run

### P0.2 Fix Validation Warning `VUID-VkGraphicsPipelineCreateInfo-Input-07904`
**Pain**: Vertex shader input Location 0 missing from vertex attribute descriptions.
**Effort**: Low (1 file)
**Impact**: Clean validation output

---

## P1 — FRICTION REDUCTION (This Week / Next Sprint)

### P1.1 GPU Frame Timer + ImGui Overlay
**Pain**: No perf data. Flying blind on which pass costs what.
**Approach**: NVRHI `TimerQuery` wrapper → `FGPUProfiler` with push/pop markers → render via existing `FImgui_Renderer`.
**Effort**: Low-Medium
**Impact**: Data-driven optimization

### P1.2 ImGui CVar Debug Browser
**Pain**: 20+ CVars exist (`r_TAA`, `r_SSAO_RadiusScale`, etc.) but zero runtime UI.
**Approach**: Iterate registered CVars, group by prefix, render sliders/checkboxes in ImGui panel.
**Effort**: Low
**Impact**: Artist-friendly tuning

### P1.3 Consolidate Per-Test Texture Loading
**Pain**: `TestRenderSponza`, `TestRTShadowsGBuffer`, `FSceneGPUData` all have copy-pasted `PendingTextures` + `UploadCmdList` logic.
**Approach**: `FSceneGPUData::UploadTexturesAsync(Materials, Device, UploadCmdList)` — one helper, all call sites.
**Effort**: Low
**Impact**: Delete ~150 lines of duplication

### P1.4 Shader Hot-Reload (Leverage FAssetLoader)
**Pain**: Change HLSL → recompile → rebuild test → relaunch. 30-60s loop.
**Approach**: 
1. Investigate `FAssetLoader`'s existing file watcher
2. Watch `*_Data/*.sblob` files for changes
3. On change: mark pipeline dirty, recreate at frame boundary via `FDeferredFrameRenderer::Initialize()`
**Effort**: Medium
**Impact**: Massive dev velocity boost
**Risk**: NVRHI pipeline recreation is destructive. Must happen at frame boundary with `waitForIdle`.

---

## P2 — STRUCTURAL BRIDGE (This Month)

### P2.1 Unified ResourceManager
**Pain**: The explore agent called this out as the **biggest structural gap**.
- Mesh/material maps live inside `FScene3DLoader`'s local `AssetLoadingContext`
- `FAssetLoader` (async generic loader) and `FScene3DLoader` are **not integrated**
- Every test reinvents asset lifetime

**Approach**: Minimal `FResourceManager`:
```cpp
class FResourceManager {
    // UUID → filepath → loaded asset
    TMap<FAssetID, FTexture*> Textures;
    TMap<FAssetID, FStaticMesh*> Meshes;
    TMap<FAssetID, FPBRMaterial*> Materials;
    
    // Hot-reload hook
    void OnFileChanged(const FPath& Path);
    
    // Reference counting
    void AddRef(FAssetID); void Release(FAssetID);
};
```
**Effort**: High (2-3 weeks)
**Impact**: Unifies `FAssetLoader` + `FScene3DLoader`, enables hot-reload for ALL assets, prerequisite for everything else

### P2.2 Central Shader Library
**Pain**: Per-test shader data dirs (`TestRenderSponza_Data/`, `TestSponzaDeferred_Data/`). Same shader compiled N times.
**Approach**: Shared `Engine/Shaders/` directory with one `ShaderMake.cfg`. Tests reference shared `.sblob` files.
**Effort**: Medium
**Impact**: Faster builds, less disk waste, shader reuse
**Blocked by**: Nothing. Can do in parallel with ResourceManager.

### P2.3 Texture Streaming MVP
**Pain**: All textures load at full resolution at init.
**Approach**: 
1. Load lowest mip (64×64) synchronously at init
2. Enqueue full-resolution decode async
3. Upload higher mips to existing texture via `writeTexture`
**Effort**: Medium
**Impact**: Faster startup, lower initial VRAM
**Blocked by**: ResourceManager (need to track which mips are resident)

---

## P3 — ADVANCED (3+ Months, After P2 Stable)

### P3.1 Minimal Render Graph
**Pain**: `FDeferredFrameRenderer` manually wires 15+ passes. Adding a pass touches 5 files.
**Why deferred**: Needs ResourceManager first (graph references textures by handle). Without it, graph nodes can't resolve resources.
**Approach**: MVP = resource declaration + auto barriers only. NO scheduler/reordering.
**Effort**: High

### P3.2 GPU Instancing + GPU Scene Graph
**Pain**: `FNode` is CPU transform hierarchy only. No GPU instancing.
**Why deferred**: Needs ResourceManager to manage instance buffers and indirect draw commands.
**Effort**: High

### P3.3 Bindless Textures
**Pain**: 8 SRV slots in deferred lighting. Scales poorly with material variety.
**Why deferred**: Must verify NVRHI fork supports `VK_EXT_descriptor_indexing`. Also needs render graph or unified binding management.
**Effort**: Medium-High

---

## CUT — NOT AVAILABLE YET

These features require infrastructure that **does not exist**. Do NOT start until prerequisites are built.

| Feature | Missing Prerequisite | When |
|---------|---------------------|------|
| ECS | Serialization system | After ResourceManager + JSON scene format |
| Animation / Skinning | Skinning GPU pipeline, animation clip format | After ECS |
| Physics | ECS to attach bodies to | After ECS |
| Forward+ / Transparency | Clustered light data structure | After many lights become a bottleneck |
| Volumetric Fog / Clouds | 3D texture infrastructure, volume rendering | After ResourceManager handles 3D textures |
| GI (DDGI / SDFGI) | Stable RT pipeline, probe volume management | After RT shadows are bulletproof |
| VSM | Shadow atlas management, page allocator | After ResourceManager |
| DLSS / FSR | No upscaling need in current test scenes | When rendering above 1080p matters |
| Level Editor | Serialization, ECS, gizmo rendering | After ECS + serialization |
| Multi-platform | No user demand | When Windows/macOS users appear |

---

## Execution Order

```
Week 1:  P0.1 (Build bugs) + P0.2 (Validation warning)
Week 2:  P1.1 (GPU profiler) + P1.2 (CVar browser) + P1.3 (Texture loading consolidation)
Week 3:  P1.4 (Shader hot-reload) + Start P2.1 (ResourceManager)
Week 4+: P2.1 (ResourceManager) — this is the big one
Month 2: P2.2 (Shader library) + P2.3 (Texture streaming MVP)
Month 3+: P3.1 (Render graph) or P3.2 (GPU instancing) based on need
```

---

## One-Sentence Summary

**Fix the build scripts and validation warning first, then build a ResourceManager to stop every test from reinventing asset lifetime, THEN consider render graphs and bindless.**
