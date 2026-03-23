# Scene Graph Migration Plan

## Overview
Migrate scene-graph module from Engine2021 (`/media/home/hangyu5/Documents/Gitrepo-My/Engine2021/engine/source/engine/scene-graph/`) into HLVM-Engine (`/Engine/Source/Runtime/Public/Renderer/Scene3D/`).

**Goal**: Reuse existing HLVM foundation files, adapt Engine2021 scene-graph code to HLVM coding style, avoid creating new files where possible.

---

## 1. Source Files Analysis

### Files to Migrate (from Engine2021):
```
scene-graph/
├── Scene3DManager.h           → Scene3D/Scene3DManager.h
├── Scene3DManager.cpp         → Scene3D/Private/Scene3DManager.cpp
├── Scene3DNode.h              → Scene3D/Scene3DNode.h
├── Scene3DNode.cpp            → Scene3D/Private/Scene3DNode.cpp
└── assimp/
    ├── AssimpHelper.h         → Scene3D/Assimp/AssimpHelper.h (already exists, empty)
    ├── AssimpSceneObject.h    → Scene3D/Assimp/AssimpSceneObject.h
    └── AssimpSceneObject.cpp  → Scene3D/Private/AssimpSceneObject.cpp
```

### Existing HLVM Foundation (already in place):
```
Scene3D/
├── Assimp/
│   ├── AssimpHelper.h         (empty - will fill)
│   └── AssimpSceneLoader.h    (empty - may not need)
├── ModelLoader.h              (empty - may integrate into manager)
├── ModelPrefab.h              (empty - may not need)
└── SceneGraph/                (empty directory)
```

---

## 2. Dependencies & Missing Components

### External Dependencies (from Engine2021):
1. **Assimp** - Already available in HLVM via vcpkg ✅
2. **GLM** - Already available in HLVM via vcpkg ✅
3. **Boost.Filesystem** - Already available in HLVM ✅

### Engine2021-Specific Dependencies (Need Adaptation):

#### 2.1 ResourceManager System
**Engine2021 Usage**:
```cpp
ResourceManager<AssimpSceneObject>::GetInstance()->TryGet(sceneNodeName)
ResourceManager<Scene3DNode>::GetInstance()->AddResource(...)
```

**HLVM Equivalent**: 
- Check if HLVM has ResourceManager in AssetManager
- Location: `/Engine/Source/Runtime/Public/AssetManager/ResourceManager.h`
- **Action**: Review HLVM ResourceManager API and adapt calls

#### 2.2 MemoryManager
**Engine2021 Usage**:
```cpp
MemoryManager::Make_shared<Mesh>(...)
MemoryManager::Make_shared<Material>(...)
```

**HLVM Equivalent**:
- HLVM uses custom allocator system (FMiMallocator, FStackAllocator)
- May need to use `MakeShared` from HLVM Common or direct `std::make_shared`
- **Action**: Check HLVM memory allocation patterns

#### 2.3 String Types
**Engine2021**: Uses `std::string`, `fs::path`
**HLVM**: Uses `FString`, `FPath`
- **Action**: Convert all string types to HLVM conventions

#### 2.4 Math Types
**Engine2021**: Uses `glm::mat4`, `Vec3f`, `Vec2f`, `Mat4`
**HLVM**: Uses `FMat4`, `FVec3`, `FVec2` (from MathGLM.h)
- **Action**: Update all math type references

#### 2.5 Logging
**Engine2021**: Uses `PRINT()`, `DEBUG_PRINT()`
**HLVM**: Uses UE5-style `HLVM_LOG(LogCategory, level, TXT(...))`
- **Action**: Convert all logging to HLVM macros

#### 2.6 Exception Handling
**Engine2021**: Uses `ENGINE_EXCEPT`, `ENGINE_EXCEPT_IF`
**HLVM**: Uses `HLVM_ASSERT`, `HLVM_ENSURE`, custom exception system
- **Action**: Replace with HLVM assertion/exception macros

#### 2.7 ECS Components (Scene3DCom, EntityDecorator, GameWorld)
**Engine2021 Usage**:
```cpp
void LoadSceneNodeToEntity(EntityDecorator rootEntity, const std::string& sceneNodeName);
rootEntity.GetComponent<Scene3DCom>();
```

**HLVM Status**: ❌ **MISSING** - HLVM may not have ECS system
- **Action**: 
  - Option A: Remove ECS-specific functionality, keep scene loading only
  - Option B: Implement minimal ECS compatibility layer
  - **Recommendation**: Option A - focus on scene graph loading, decouple from ECS

#### 2.8 Animation/Skeleton/Material/Mesh Classes
**Engine2021 Dependencies**:
```cpp
Skeleton::LoadSkeleton(aiscene, sceneName)
Animation3D::LoadAnimation(aiscene, sceneName)
Mesh, MeshData, Material classes
```

**HLVM Status**: Need to verify existence
- **Action**: Search HLVM codebase for these classes
  - If exist: Adapt to HLVM APIs
  - If missing: Create stub implementations or defer to rendering module

---

## 3. File-by-File Migration Strategy

### 3.1 AssimpHelper.h
**Source**: Engine2021 `assimp/AssimpHelper.h`  
**Target**: `Scene3D/Assimp/AssimpHelper.h` (replace empty file)

**Changes Required**:
- ✅ Keep matrix conversion functions (`Assimp2Glm`, `Glm2Assimp`)
- ✅ Keep debug print functions (`ShowMesh`, `ShowAnimation`, `ShowBoneHierarchy`)
- ⚠️ **Remove dependency on `engine/core/EngineCore.h`** → Use HLVM `Core/Log.h`
- ⚠️ **Remove dependency on `engine/math/Geommath.h`** → Use HLVM `Math/GeomMath.h`
- ✅ Adapt to HLVM include structure

**HLVM Adaptation**:
```cpp
#pragma once

#include "Math/MathGLM.h"  // HLVM GLM wrapper
#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Matrix conversion functions (keep as-is, just adapt types)
// Debug print functions (adapt PRINT to HLVM_LOG)
```

---

### 3.2 AssimpSceneObject.h/.cpp
**Source**: Engine2021 `assimp/AssimpSceneObject.*`  
**Target**: `Scene3D/Assimp/AssimpSceneObject.h` + `Scene3D/Private/AssimpSceneObject.cpp`

**Changes Required**:
- ✅ Class wraps `aiImportFile` and `aiReleaseImport`
- ⚠️ **Remove dependency on `MemoryManager`** → Use HLVM allocator or direct construction
- ⚠️ **Convert exception**: `ENGINE_EXCEPT_IF` → `HLVM_ENSURE` or custom error handling
- ⚠️ **String conversion**: `fs::path` → `FPath`, `std::string` → `FString`
- ⚠️ **Logging**: `DEBUG_PRINT` → `HLVM_LOG(LogScene3D, debug, ...)`

**Structure**:
```cpp
// AssimpSceneObject.h
#pragma once

#include "AssimpHelper.h"
#include "Core/Assert.h"      // HLVM assertions
#include "Core/Log.h"         // HLVM logging
#include "Platform/FileSystem.h" // For FPath

class FAssimpSceneObject
{
public:
    // Factory method
    static std::shared_ptr<FAssimpSceneObject> LoadFromFile(const FPath& Path);
    
    // Constructor
    explicit FAssimpSceneObject(const FPath& Path);
    ~FAssimpSceneObject();
    
    // Accessors
    const aiScene* GetScene() const noexcept;
    const aiNode* GetRoot() const noexcept;

private:
    const aiScene* m_Scene{ nullptr };
};
```

---

### 3.3 Scene3DNode.h/.cpp
**Source**: Engine2021 `Scene3DNode.*`  
**Target**: `Scene3D/Scene3DNode.h` + `Scene3D/Private/Scene3DNode.cpp`

**Changes Required**:
- ✅ Core scene node data structure (mesh hierarchy, animation data)
- ⚠️ **Container types**: `LongMarch_Vector` → `TVector` (HLVM boost container wrapper)
- ⚠️ **Smart pointers**: Keep `std::shared_ptr` (HLVM compatible)
- ⚠️ **String types**: `std::string` → `FString`
- ⚠️ **Remove ECS dependencies**: Remove `friend Animation3DCom`
- ⚠️ **Skeleton/Animation types**: Verify HLVM equivalents or create stubs

**Key Structures**:
```cpp
// MeshHierarchy: Tree of (level, Mesh) pairs
using MeshHierarchy = TVector<std::pair<uint32_t, std::shared_ptr<FMesh>>>;

// AnimationData: Bone inverse transforms
struct AnimationData
{
    // Bone_Transform_LUT needs definition - likely unordered_map<string, FMat4>
    THashMap<FString, FMat4> Bone_InverseFinalTransform_LUT;
};
```

**Methods to Implement**:
- `Copy()` - Deep copy with new material instances
- `GetAllMesh()` - Extract all mesh data
- `GetAllMaterial()` - Extract all materials
- `ModifyAllMaterial(callback)` - Apply modification to all materials
- `SetInverseFinalBoneTransform()` / `GetInverseFinalBoneTransform()` - Animation support

---

### 3.4 Scene3DManager.h/.cpp
**Source**: Engine2021 `Scene3DManager.*`  
**Target**: `Scene3D/Scene3DManager.h` + `Scene3D/Private/Scene3DManager.cpp`

**Changes Required**:
- ✅ Singleton pattern (keep)
- ⚠️ **ResourceManager integration**: Adapt to HLVM ResourceManager API
- ⚠️ **Remove ECS methods**: `LoadSceneNodeToEntity()` requires ECS - defer or remove
- ⚠️ **Material loading**: Adapt material texture loading to HLVM asset system
- ⚠️ **Mesh vertex formats**: `MESH_VERTEX_DATA_FORMAT` conditional compilation
  - HLVM may have different vertex format requirements
  - **Action**: Check HLVM mesh/vertex format, simplify if needed

**Core Functionality**:
```cpp
class FScene3DManager
{
private:
    FScene3DManager() = default;
    HLVM_NONCOPYABLE(FScene3DManager);

public:
    // Singleton
    static FScene3DManager* GetInstance();

    // Scene loading
    void LoadSceneNodeFromAssimp(const FString& SceneNodeName);
    // LoadSceneNodeToEntity - REMOVE (ECS dependency)

private:
    // Recursive scene graph traversal
    void RecurseLoad(FScene3DNode& SceneData, 
                     const FString& SceneName, 
                     const FPath& SceneDir,
                     const aiScene* AIScene, 
                     const aiNode* Node,
                     const aiMatrix4x4& ParentTr, 
                     uint32_t Level);
};
```

**Critical Adaptations**:
1. **Material Loading**: Convert Assimp material properties to HLVM PBR material format
2. **Mesh Processing**: Vertex transformation, normal/tangent handling
3. **Bone/Skeleton**: If animation support needed, implement skeleton loading
4. **Resource Registration**: Use HLVM ResourceManager for caching

---

## 4. Missing HLVM Components (Blockers)

### 4.1 Mesh & MeshData Classes ❌
**Status**: Not found in HLVM Runtime  
**Action Required**:
- Option A: Create in `Scene3D/Mesh/Mesh.h` and `MeshData.h`
- Option B: Use existing rendering module mesh classes
- **Recommendation**: Check with rendering team, may already exist in NVRHI integration

### 4.2 Material Class ❌
**Status**: HLVM has `IMaterial.h` and `PBRMaterial.h` in `Renderer/Material/`  
**Action**: Use `FPBRMaterial` from HLVM, adapt Assimp material loading

### 4.3 Skeleton & Animation3D Classes ❌
**Status**: Not found in HLVM  
**Action**:
- **Option A (Recommended)**: Defer animation support - load static meshes only
- **Option B**: Create minimal skeleton/animation stubs
- **Decision**: Implement Option A first, add animation later

### 4.4 ResourceManager API ⚠️
**Status**: Exists in HLVM (`AssetManager/ResourceManager.h`)  
**Action**: Verify API compatibility:
```cpp
// Check if HLVM has:
ResourceManager<T>::GetInstance()->TryGet(name)
ResourceManager<T>::GetInstance()->AddResource(name, path, shared_ptr)
ResourceManager<T>::GetInstance()->Has(name)
```

---

## 5. Implementation Phases

### Phase 1: Foundation (No Dependencies)
1. **AssimpHelper.h** - Matrix conversions, debug utilities
2. **AssimpSceneObject.h/.cpp** - Assimp file loading wrapper
3. **Basic types** - Define missing types (Mesh, MeshData if needed)

### Phase 2: Core Scene Graph
1. **Scene3DNode.h/.cpp** - Scene node data structure
2. **Material adaptation** - Map Assimp materials to HLVM PBRMaterial
3. **Mesh processing** - Vertex/index extraction from Assimp

### Phase 3: Manager & Integration
1. **Scene3DManager.h/.cpp** - Scene loading orchestration
2. **ResourceManager integration** - Hook into HLVM asset system
3. **Testing** - Load sample glTF/FBX files

### Phase 4: Advanced Features (Optional)
1. **Animation support** - Skeleton, Animation3D classes
2. **ECS integration** - If HLVM has/wants ECS
3. **Optimization** - LOD, instancing, etc.

---

## 6. Coding Style Adaptation

### 6.1 Naming Conventions
**Engine2021** → **HLVM**:
- `Scene3DNode` → `FScene3DNode` (F prefix for classes)
- `sceneNodeName` → `SceneNodeName` (camelCase → PascalCase for members)
- `m_aiscene` → `m_Scene` (HLVM member variable style)
- `LongMarch_Vector` → `TVector` (HLVM container types)

### 6.2 Include Order
```cpp
// 1. HLVM Common (Public first)
#include "Core/Log.h"
#include "Core/Assert.h"
#include "Core/Container/ContainerDefinition.h"

// 2. HLVM Runtime
#include "Math/MathGLM.h"
#include "Renderer/Material/PBRMaterial.h"

// 3. Third-party
#include <assimp/Importer.hpp>
#include <boost/filesystem.hpp>

// 4. Standard Library
#include <memory>
#include <string>
```

### 6.3 Logging
**Before (Engine2021)**:
```cpp
PRINT("Loading node : " + sceneName);
DEBUG_PRINT("Reading " + _path);
```

**After (HLVM)**:
```cpp
DECLARE_LOG_CATEGORY(LogScene3D)

HLVM_LOG(LogScene3D, info, TXT("Loading node: {}"), SceneName);
HLVM_LOG(LogScene3D, debug, TXT("Reading file: {}"), Path.ToString());
```

### 6.4 Error Handling
**Before (Engine2021)**:
```cpp
ENGINE_EXCEPT_IF(aiface->mNumIndices != 3, L"Triangle mesh has wrong indices");
```

**After (HLVM)**:
```cpp
HLVM_ENSURE_F(aiface->mNumIndices == 3, 
              TXT("Triangle mesh {} has {} indices instead of 3"), 
              MeshName, aiface->mNumIndices);
```

### 6.5 String Handling
**Before (Engine2021)**:
```cpp
std::string name = node->mName.C_Str();
fs::path path = sceneDir / filename;
```

**After (HLVM)**:
```cpp
FString Name = FString(UTF8_TO_TCHAR(node->mName.C_Str()));
FPath Path = FPath(SceneDir) / FPath(Filename);
```

### 6.6 Memory Allocation
**Before (Engine2021)**:
```cpp
auto material = MemoryManager::Make_shared<Material>();
auto meshdata = MemoryManager::Make_shared<MeshData>();
```

**After (HLVM)**:
```cpp
// Option 1: Direct shared_ptr
auto Material = std::make_shared<FPBRMaterial>();

// Option 2: If HLVM has MakeShared
auto Material = MakeShared<FPBRMaterial>();
```

---

## 7. File Structure (Final)

```
Engine/Source/Runtime/Public/Renderer/Scene3D/
├── Assimp/
│   ├── AssimpHelper.h              # Matrix conversions, debug utils
│   └── AssimpSceneObject.h         # Assimp file loading wrapper
├── Scene3DNode.h                   # Scene node data structure
└── Scene3DManager.h                # Scene loading singleton

Engine/Source/Runtime/Private/Renderer/Scene3D/
├── AssimpSceneObject.cpp           # AssimpSceneObject implementation
├── Scene3DNode.cpp                 # Scene3DNode implementation
└── Scene3DManager.cpp              # Scene3DManager implementation
```

**Note**: HLVM uses Public/Private separation. Headers in Public/, implementations in Private/.

---

## 8. Open Questions & Decisions Needed

### 8.1 Mesh & Vertex Format
**Question**: What vertex format does HLVM rendering pipeline expect?
- Engine2021 has 5 formats (`MESH_VERTEX_DATA_FORMAT` 0-4)
- HLVM may have different requirements
- **Action**: Check HLVM mesh/vertex format or simplify to single format

### 8.2 Animation Support
**Question**: Should animation/skeleton loading be included in initial migration?
- **Recommendation**: No - defer to Phase 4
- **Reason**: Requires Skeleton, Animation3D classes not present in HLVM
- **Action**: Strip animation code or create stub implementations

### 8.3 ECS Integration
**Question**: Does HLVM have ECS system for `LoadSceneNodeToEntity()`?
- **Current Status**: Unknown - need to verify
- **Recommendation**: Remove ECS methods initially, add later if needed

### 8.4 Resource Management
**Question**: How does HLVM ResourceManager handle type registration?
- **Action**: Verify HLVM ResourceManager API supports Scene3DNode type

### 8.5 Material System
**Question**: Does HLVM PBRMaterial support all Assimp material properties?
- Albedo, Metallic, Roughness, Normal, AO - likely supported
- **Action**: Map Assimp material properties to FPBRMaterial fields

---

## 9. Next Steps

1. **Verify Missing Components**:
   - Search HLVM for Mesh, MeshData classes
   - Check ResourceManager API
   - Verify PBRMaterial structure

2. **Create Stub Implementations** (if needed):
   - Simple Mesh/MeshData structures
   - Basic Skeleton/Animation stubs (or skip)

3. **Start Implementation**:
   - Phase 1: AssimpHelper, AssimpSceneObject
   - Phase 2: Scene3DNode with basic mesh loading
   - Phase 3: Scene3DManager integration

4. **Testing Strategy**:
   - Create simple test with glTF model
   - Verify scene graph structure
   - Test material loading
   - Validate mesh data integrity

---

## 10. Risk Assessment

### High Risk:
- ❌ **Missing Mesh/MeshData classes** - Could block entire migration
- ❌ **ResourceManager incompatibility** - May require significant adaptation

### Medium Risk:
- ⚠️ **Material property mapping** - May lose some material fidelity
- ⚠️ **Vertex format conversion** - May need rendering pipeline adjustments

### Low Risk:
- ✅ **Assimp integration** - Straightforward wrapper adaptation
- ✅ **Scene graph structure** - Simple data structure migration
- ✅ **String/logging conversion** - Mechanical find-replace work
- ✅ **Vertex format** - Simple FP32 format confirmed
- ✅ **Animation** - DEFERRED (not needed)
- ✅ **ECS** - STRIPPED (not needed)
- ✅ **Assimp integration** - Straightforward wrapper adaptation
- ✅ **Scene graph structure** - Simple data structure migration
- ✅ **String/logging conversion** - Mechanical find-replace work

---

## 11. Success Criteria

Migration is complete when:
1. ✅ Can load glTF/FBX files via Assimp
2. ✅ Scene graph structure preserved (hierarchy, transforms)
3. ✅ Materials loaded with PBR properties
4. ✅ Meshes loaded with correct vertex/index data
5. ✅ Integration with HLVM ResourceManager
6. ✅ No Engine2021 dependencies remain
7. ✅ Follows HLVM coding style (naming, logging, error handling)
8. ✅ Compiles without errors/warnings in HLVM build system

---

## Appendix A: Type Mapping Reference

| Engine2021 | HLVM Equivalent | Notes |
|------------|----------------|-------|
| `std::string` | `FString` | Use `FString(UTF8_TO_TCHAR(c_str))` |
| `fs::path` | `FPath` | Boost filesystem wrapper |
| `LongMarch_Vector<T>` | `TVector<T>` | Boost container wrapper |
| `MemoryManager::Make_shared<T>` | `std::make_shared<T>` | Or HLVM MakeShared |
| `PRINT()` | `HLVM_LOG(Log, info, ...)` | UE5-style logging |
| `ENGINE_EXCEPT` | `HLVM_ENSURE` | Assertion/exception |
| `glm::mat4` | `FMat4` | GLM wrapper |
| `Vec3f` | `FVec3` | GLM vec3 |
| `Mat4` | `FMat4` | GLM mat4 |

---

**Status**: Plan UPDATED with user feedback. Ready to implement.
**User Decisions**:
- ✅ Mesh: Implement existing stub in `Renderer/Mesh/`
- ✅ Animation: DEFERRED (not needed at this stage)
- ✅ ECS: STRIPPED entirely
- ✅ Vertex format: Simple FP32 (position, normal, UV, tangent)

**Estimated Effort**: 
- Phase 1-3 (Core): 3-5 days total

**Recommendation**: Start implementation with Phase 1 (Assimp foundation).
**Estimated Effort**: 
- Phase 1-2 (Core): 2-3 days
- Phase 3 (Integration): 1-2 days
- Phase 4 (Animation): 2-3 days (optional)

**Recommendation**: Start with Phase 1-2, validate approach, then proceed to Phase 3.
