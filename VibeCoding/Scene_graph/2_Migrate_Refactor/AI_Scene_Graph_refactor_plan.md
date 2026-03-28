# Scene Graph Refactoring Plan

## Overview
Refactor the scene graph migration code to improve architecture by:
1. Renaming `Scene3DManager` to `Scene3DLoader` and simplifying its interface
2. Extracting base interfaces for mesh and material systems
3. Consolidating mesh classes into `StaticMesh` with `IMesh` interface

## 1. Scene3DManager → Scene3DLoader Refactoring

### Current Issues:
- `Scene3DManager` is a singleton with unnecessary ResourceManager dependencies
- Complex lambda-based loading functions with compilation errors
- `HLVM_NONCOPYABLE` macro causing warnings
- Uses deprecated glTF-specific material keys

### Proposed Changes:

#### File Renames:
- `/Engine/Source/Runtime/Public/Renderer/Scene3D/Scene3DManager.h` → `Scene3DLoader.h`
- `/Engine/Source/Runtime/Private/Renderer/Scene3D/Scene3DManager.cpp` → `Scene3DLoader.cpp`

#### New Interface (`Scene3DLoader.h`):
```cpp
class FScene3DLoader
{
public:
    // Simple static loader functions
    static std::shared_ptr<FScene3DNode> LoadFromFile(const FPath& ScenePath);
    static std::shared_ptr<FScene3DNode> LoadFromAssimp(const FPath& ScenePath, const FString& SceneName = TXT(""));
    
private:
    // Private recursive loading helper
    static void RecurseLoad(
        FScene3DNode& SceneData,
        const FString& SceneName,
        const FPath& SceneDir,
        const aiScene* AIScene,
        const aiNode* Node,
        const aiMatrix4x4& ParentTr,
        uint32_t Level
    );
};
```

#### Key Simplifications:
1. Remove singleton pattern - use static methods
2. Remove ResourceManager dependencies
3. Fix material key usage (use standard Assimp keys, not glTF-specific)
4. Return `std::shared_ptr<FScene3DNode>` directly
5. Remove `HLVM_NONCOPYABLE` macro

## 2. Mesh System Refactoring (Mesh/MeshData → StaticMesh + IMesh)

### Current Structure:
- `FMesh`: Lightweight wrapper around `FMeshData`
- `FMeshData`: Raw vertex/index data container
- No interface for polymorphism

### Proposed Changes:

#### New Files:
1. `/Engine/Source/Runtime/Public/Renderer/Mesh/IMesh.h` - Base interface
2. `/Engine/Source/Runtime/Public/Renderer/Mesh/StaticMesh.h` - Combined `FMesh` + `FMeshData`

#### IMesh Interface (`IMesh.h`):
```cpp
class IMesh
{
public:
    virtual ~IMesh() = default;
    
    virtual const FString& GetName() const = 0;
    virtual bool IsValid() const = 0;
    virtual TUINT64 NumVertices() const = 0;
    virtual TUINT64 NumIndices() const = 0;
    virtual TUINT64 NumTriangles() const = 0;
    
    // Optional: Get raw data access
    virtual const TVector<FVertex>& GetVertices() const = 0;
    virtual const TVector<uint32_t>& GetIndices() const = 0;
};
```

#### StaticMesh Class (`StaticMesh.h`):
```cpp
class FStaticMesh : public IMesh
{
public:
    // Vertex format (same as current FVertex)
    struct FVertex { ... };
    
    // Constructors
    FStaticMesh();
    explicit FStaticMesh(const FString& InName);
    FStaticMesh(const FString& InName, const TVector<FVertex>& Vertices, const TVector<uint32_t>& Indices);
    
    // IMesh implementation
    const FString& GetName() const override;
    bool IsValid() const override;
    TUINT64 NumVertices() const override;
    TUINT64 NumIndices() const override;
    TUINT64 NumTriangles() const override;
    const TVector<FVertex>& GetVertices() const override;
    const TVector<uint32_t>& GetIndices() const override;
    
    // Helper methods (from FMeshData)
    void Clear();
    void ReserveVertices(TSIZE Count);
    void ReserveIndices(TSIZE Count);
    void AddVertex(const FVertex& Vertex);
    void AddTriangle(uint32_t Index0, uint32_t Index1, uint32_t Index2);
    
private:
    FString Name;
    TVector<FVertex> Vertices;
    TVector<uint32_t> Indices;
};
```

#### File Migration:
- Delete: `Mesh.h`, `MeshData.h`
- Create: `StaticMesh.h` (combines both)
- Create: `IMesh.h` (interface)

## 3. Material System Refactoring (PBRMaterial → IMaterial)

### Current Structure:
- `FPBRMaterial`: Concrete PBR material implementation
- No interface for polymorphism

### Proposed Changes:

#### New Files:
1. `/Engine/Source/Runtime/Public/Renderer/Material/IMaterial.h` - Base interface

#### IMaterial Interface (`IMaterial.h`):
```cpp
class IMaterial
{
public:
    virtual ~IMaterial() = default;
    
    virtual const FString& GetName() const = 0;
    virtual bool HasTexture(ETextureType Type) const = 0;
    virtual FPath GetTexturePath(ETextureType Type) const = 0;
    virtual FString GetTextureName(ETextureType Type) const = 0;
    
    // Common material properties
    virtual FVec3 GetAlbedoColor() const = 0;
    virtual float GetMetallic() const = 0;
    virtual float GetRoughness() const = 0;
    
    // Texture type enum (moved from FPBRMaterial)
    enum class ETextureType : uint8_t
    {
        Albedo = 0,
        Normal,
        Metallic,
        Roughness,
        AmbientOcclusion,
        Count
    };
};
```

#### FPBRMaterial Updates:
- Keep existing `FPBRMaterial` class
- Make it inherit from `IMaterial`
- Implement all virtual methods
- Move `ETextureType` enum to interface

## 4. Build Impact Analysis

### Files to Modify:
1. `Scene3DManager.h` → `Scene3DLoader.h` (rename + refactor)
2. `Scene3DManager.cpp` → `Scene3DLoader.cpp` (rename + refactor)
3. `Mesh.h` → Delete
4. `MeshData.h` → Delete  
5. Create `StaticMesh.h` (new)
6. Create `IMesh.h` (new)
7. Create `IMaterial.h` (new)
8. Update `PBRMaterial.h` to inherit from `IMaterial`
9. Update `Scene3DNode.h/cpp` to use new interfaces
10. Update `AssimpSceneObject.cpp` to use new loader

### Compilation Dependencies:
- All files using `FMesh` → use `FStaticMesh` or `IMesh`
- All files using `FPBRMaterial` → use `IMaterial` where appropriate
- Scene loading code → use `FScene3DLoader::LoadFromFile()`

## 5. Implementation Steps

### Phase 1: Create Base Interfaces
1. Create `IMesh.h` with interface definition
2. Create `IMaterial.h` with interface definition
3. Update `PBRMaterial.h` to inherit from `IMaterial`

### Phase 2: Create StaticMesh
1. Create `StaticMesh.h` combining `Mesh.h` and `MeshData.h`
2. Implement `IMesh` interface
3. Test compilation with existing code

### Phase 3: Refactor Scene3DLoader
1. Rename `Scene3DManager.h/cpp` to `Scene3DLoader.h/cpp`
2. Rewrite as static class without ResourceManager
3. Fix material key usage (standard Assimp keys)
4. Update return type to `std::shared_ptr<FScene3DNode>`

### Phase 4: Update Dependencies
1. Update `Scene3DNode.h/cpp` to use new interfaces
2. Update `AssimpSceneObject.cpp` to use new loader
3. Update any other files referencing old classes

### Phase 5: Cleanup
1. Delete `Mesh.h` and `MeshData.h`
2. Run build tests
3. Verify scene loading works

## 6. Risk Mitigation

### Potential Issues:
1. **Circular Dependencies**: Interface headers must not include concrete implementations
2. **Build Breakage**: Need to update all references simultaneously
3. **Performance**: Virtual calls may have minor overhead (acceptable for scene graph)

### Mitigation Strategies:
1. Use forward declarations where possible
2. Implement in phases with intermediate builds
3. Profile if performance becomes an issue (unlikely for loading code)

## 7. Success Criteria

1. ✅ All compilation errors resolved
2. ✅ `./Build.sh --Config=Debug` succeeds
3. ✅ Scene loading works with sample glTF/FBX files
4. ✅ Code follows HLVM coding style
5. ✅ No ResourceManager dependencies in scene loading
6. ✅ Base interfaces allow future extensions (animated meshes, different material types)

## 8. Detailed Interface Designs (HLVM Style)

### IMesh Interface (IMesh.h)
```cpp
/**
 * @brief Base interface for all mesh types
 * 
 * Provides common mesh operations for both static and animated meshes.
 * Follows HLVM naming conventions: I prefix for interfaces.
 */
class IMesh
{
public:
    virtual ~IMesh() = default;
    
    //! Get mesh name
    virtual const FString& GetName() const = 0;
    
    //! Check if mesh has valid data
    virtual bool IsValid() const = 0;
    
    //! Get vertex count
    virtual TUINT64 NumVertices() const = 0;
    
    //! Get index count
    virtual TUINT64 NumIndices() const = 0;
    
    //! Get triangle count
    virtual TUINT64 NumTriangles() const = 0;
    
    //! Optional: Get raw vertex data (for static meshes)
    virtual const TVector<FVertex>& GetVertices() const = 0;
    
    //! Optional: Get raw index data (for static meshes)
    virtual const TVector<uint32_t>& GetIndices() const = 0;
};
```

### IMaterial Interface (IMaterial.h)
```cpp
/**
 * @brief Base interface for all material types
 * 
 * Common material properties and texture operations.
 * Texture type enum moved from FPBRMaterial to interface.
 */
class IMaterial
{
public:
    virtual ~IMaterial() = default;
    
    //! Texture type enumeration
    enum class ETextureType : uint8_t
    {
        Albedo = 0,
        Normal,
        Metallic,
        Roughness,
        AmbientOcclusion,
        Count
    };
    
    //! Get material name
    virtual const FString& GetName() const = 0;
    
    //! Check if material has texture of specified type
    virtual bool HasTexture(ETextureType Type) const = 0;
    
    //! Get texture path for specified type
    virtual FPath GetTexturePath(ETextureType Type) const = 0;
    
    //! Get texture name for specified type
    virtual FString GetTextureName(ETextureType Type) const = 0;
    
    //! Common PBR properties
    virtual FVec3 GetAlbedoColor() const = 0;
    virtual float GetMetallic() const = 0;
    virtual float GetRoughness() const = 0;
};
```

### FStaticMesh Class (StaticMesh.h)
```cpp
/**
 * @brief Static mesh implementation with vertex/index data
 * 
 * Combines functionality of old FMesh and FMeshData.
 * Follows HLVM naming: F prefix for classes.
 */
class FStaticMesh : public IMesh
{
public:
    //! Vertex format (44 bytes)
    struct FVertex
    {
        FVec3 Position;  // 12 bytes
        FVec3 Normal;    // 12 bytes
        FVec2 UV;        // 8 bytes
        FVec3 Tangent;   // 12 bytes
        
        FVertex() = default;
        FVertex(const FVec3& InPosition, const FVec3& InNormal, 
                const FVec2& InUV, const FVec3& InTangent);
    };
    
    using VertexContainer = TVector<FVertex>;
    using IndexContainer = TVector<uint32_t>;
    
public:
    FStaticMesh() = default;
    explicit FStaticMesh(const FString& InName);
    FStaticMesh(const FString& InName, const VertexContainer& Vertices, 
                const IndexContainer& Indices);
    ~FStaticMesh() = default;
    
    // IMesh implementation
    const FString& GetName() const override;
    bool IsValid() const override;
    TUINT64 NumVertices() const override;
    TUINT64 NumIndices() const override;
    TUINT64 NumTriangles() const override;
    const VertexContainer& GetVertices() const override;
    const IndexContainer& GetIndices() const override;
    
    // Helper methods
    void Clear();
    void ReserveVertices(TSIZE Count);
    void ReserveIndices(TSIZE Count);
    void AddVertex(const FVertex& Vertex);
    void AddTriangle(uint32_t Index0, uint32_t Index1, uint32_t Index2);
    
private:
    FString Name;
    VertexContainer Vertices;
    IndexContainer Indices;
};
```

### FScene3DLoader Class (Scene3DLoader.h)
```cpp
/**
 * @brief Static scene loader for 3D scene files
 * 
 * Simplified version of Scene3DManager without ResourceManager dependencies.
 * Uses static methods instead of singleton pattern.
 */
class FScene3DLoader
{
public:
    //! Load scene from file path
    static std::shared_ptr<FScene3DNode> LoadFromFile(const FPath& ScenePath);
    
    //! Load scene with custom name
    static std::shared_ptr<FScene3DNode> LoadFromAssimp(
        const FPath& ScenePath, 
        const FString& SceneName = TXT("")
    );
    
private:
    //! Recursive scene graph loading helper
    static void RecurseLoad(
        FScene3DNode& SceneData,
        const FString& SceneName,
        const FPath& SceneDir,
        const aiScene* AIScene,
        const aiNode* Node,
        const aiMatrix4x4& ParentTr,
        uint32_t Level
    );
};
```

### FPBRMaterial Updates
```cpp
// Change inheritance
class FPBRMaterial : public IMaterial
{
    // Existing implementation...
    // Add override keywords for IMaterial methods
    const FString& GetName() const override;
    bool HasTexture(ETextureType Type) const override;
    FPath GetTexturePath(ETextureType Type) const override;
    FString GetTextureName(ETextureType Type) const override;
    FVec3 GetAlbedoColor() const override;
    float GetMetallic() const override;
    float GetRoughness() const override;
};
```

## 9. File Changes Summary

### Files to Create:
1. `/Engine/Source/Runtime/Public/Renderer/Mesh/IMesh.h`
2. `/Engine/Source/Runtime/Public/Renderer/Mesh/StaticMesh.h`
3. `/Engine/Source/Runtime/Public/Renderer/Material/IMaterial.h`
4. `/Engine/Source/Runtime/Public/Renderer/Scene3D/Scene3DLoader.h`
5. `/Engine/Source/Runtime/Private/Renderer/Scene3D/Scene3DLoader.cpp`

### Files to Modify:
1. `/Engine/Source/Runtime/Public/Renderer/Material/PBRMaterial.h` (add inheritance)
2. `/Engine/Source/Runtime/Public/Renderer/Scene3D/Scene3DNode.h` (update types)
3. `/Engine/Source/Runtime/Private/Renderer/Scene3D/Scene3DNode.cpp` (update types)
4. `/Engine/Source/Runtime/Private/Renderer/Scene3D/AssimpSceneObject.cpp` (update loader usage)

### Files to Delete:
1. `/Engine/Source/Runtime/Public/Renderer/Mesh/Mesh.h`
2. `/Engine/Source/Runtime/Public/Renderer/Mesh/MeshData.h`
3. `/Engine/Source/Runtime/Public/Renderer/Scene3D/Scene3DManager.h`
4. `/Engine/Source/Runtime/Private/Renderer/Scene3D/Scene3DManager.cpp`

## 10. Implementation Order

1. **Create interfaces first** (IMesh.h, IMaterial.h)
2. **Update PBRMaterial** to inherit from IMaterial
3. **Create StaticMesh** class
4. **Create Scene3DLoader** class
5. **Update Scene3DNode** to use new interfaces
6. **Update AssimpSceneObject** to use new loader
7. **Delete old files**
8. **Test compilation**

This plan follows HLVM coding conventions and addresses all user requirements from AI_task.md.