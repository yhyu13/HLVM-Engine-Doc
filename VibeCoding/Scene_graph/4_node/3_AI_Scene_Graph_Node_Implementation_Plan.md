# HLVM-Engine Scene Graph Node Implementation Plan

## Goal 5 Implementation Strategy

**Task from AI_task.md**: 
1. "base on previous plan, you should impl Node class under /home/hangyu5/Documents/Gitrepo-My/HLVM-Engine/Engine/Source/Runtime/Public/Renderer/SceneGraph"
2. "class Node should use SceneNode3D as shared ptr member and apply scene-graph features within."
3. "Also impl the perspective camera class"

## Current Codebase Analysis

### Existing Files
1. **FScene3DNode** (`/Engine/Source/Runtime/Public/Renderer/Scene3D/Scene3DNode.h`)
   - Data container for loaded scene meshes and materials
   - No parent-child hierarchy or transform functionality
   - Used by `FScene3DLoader` for scene file loading

2. **FPerspectiveCamera** (`/Engine/Source/Runtime/Public/Renderer/Scene3D/FPerspectiveCamera.h`)
   - Inherits from `FScene3DNode`
   - Has camera properties (FOV, aspect, near/far)
   - Has camera matrices (projection, view, viewProjection, camera)
   - References missing methods: `UpdateWorldTransform()`, `GetRotation()`, `GetDecomposedTransform()`

3. **SceneGraph Directory** (`/Engine/Source/Runtime/Public/Renderer/SceneGraph/`)
   - Empty directory (needs to be populated)

### External Source Patterns (scenegraph-demo)
1. **Node Class**:
   - Parent-child hierarchy with `shared_ptr<Node> _parent` and `vector<unique_ptr<Node>> children`
   - Dirty flag optimization (`DirtyState::CLEAN/DIRTY`)
   - Separate TRS components: `position`, `rotation` (Euler), `scale`
   - Transform methods: `updateWorldTransform()`, `updateLocalTransform()`
   - Template `addChild<ChildType>()` method
   - `removeFromParent()` method
   - `getDecomposedTransform()` using `glm::decompose`

2. **PerspectiveCamera Class**:
   - Extends `Node`
   - Camera matrices: `projectionMatrix`, `viewMatrix`, `viewProjectionMatrix`, `cameraMatrix`
   - Overrides `updateWorldTransform()` to update camera matrices
   - View matrix computed via `lookAt()` with forward/right/up vectors

## Implementation Decision

**Option 1**: Enhance existing `FScene3DNode` with scene-graph features
- **Pros**: Maintains existing code, `FPerspectiveCamera` already inherits from it
- **Cons**: `FScene3DNode` is designed as data container, not hierarchical node

**Option 2**: Create new `FNode` class in SceneGraph directory
- **Pros**: Clean separation, follows SRP, matches external source pattern
- **Cons**: Need to update `FPerspectiveCamera` inheritance

**Recommended Approach**: **Option 2 - Create new FNode class**
- Create `FNode` in SceneGraph directory with scene-graph features
- Keep `FScene3DNode` as data container for loaded scenes
- Update `FPerspectiveCamera` to inherit from `FNode` instead of `FScene3DNode`
- `FNode` can contain `std::shared_ptr<FScene3DNode>` as member (per task requirement)

## Detailed Implementation Plan

### Phase 1: Create FNode Base Class

**File**: `/Engine/Source/Runtime/Public/Renderer/SceneGraph/FNode.h`

```cpp
// FNode - Hierarchical scene graph node with transform hierarchy
class FNode : public std::enable_shared_from_this<FNode>
{
public:
    enum class EDirtyState { Clean, Dirty };
    
    // Transform components
    FVec3 Position{0.0f, 0.0f, 0.0f};
    FVec3 Rotation{0.0f, 0.0f, 0.0f};  // Euler angles in radians
    FVec3 Scale{1.0f, 1.0f, 1.0f};
    
    // Hierarchy
    std::shared_ptr<FNode> Parent;
    TVector<std::unique_ptr<FNode>> Children;
    
    // Transforms
    FMat4 LocalTransform{1.0f};
    FMat4 WorldTransform{1.0f};
    EDirtyState DirtyState = EDirtyState::Dirty;
    
    // Scene data (per task requirement)
    std::shared_ptr<FScene3DNode> SceneData;
    
    // Constructors
    FNode();
    explicit FNode(const FString& Name);
    FNode(const FString& Name, const FVec3& Position, const FVec3& Rotation, const FVec3& Scale);
    
    // Transform methods
    void UpdateWorldTransform();
    void UpdateLocalTransform();
    void MarkDirty();
    
    // Hierarchy methods
    template<typename ChildType, typename... Args>
    ChildType& AddChild(Args&&... args);
    
    void RemoveFromParent();
    
    // Getters/Setters
    const FString& GetName() const { return Name; }
    void SetPosition(const FVec3& NewPosition);
    void SetRotation(const FVec3& NewRotation);
    void SetScale(const FVec3& NewScale);
    
    // Decomposed transform
    struct FDecomposedTransform
    {
        FVec3 Translation;
        FVec3 Rotation;  // Quaternion or Euler
        FVec3 Scale;
        FVec3 Skew;
        FVec4 Perspective;
    };
    
    FDecomposedTransform GetDecomposedTransform() const;
    
private:
    FString Name{TXT("UnnamedNode")};
};
```

**File**: `/Engine/Source/Runtime/Private/Renderer/SceneGraph/FNode.cpp`
- Implement transform update logic with dirty flag propagation
- Implement `AddChild` template method
- Implement `GetDecomposedTransform()` using GLM decompose

### Phase 2: Update FPerspectiveCamera

**File**: `/Engine/Source/Runtime/Public/Renderer/Scene3D/FPerspectiveCamera.h`
- Change inheritance: `class FPerspectiveCamera : public FNode`
- Remove duplicate transform methods (inherit from FNode)
- Keep camera-specific properties and matrices

**File**: `/Engine/Source/Runtime/Private/Renderer/Scene3D/FPerspectiveCamera.cpp`
- Update `UpdateWorldTransform()` to call parent update then update camera matrices
- Fix method calls to use FNode interface

### Phase 3: Create SceneGraph Directory Structure

```
/Engine/Source/Runtime/Public/Renderer/SceneGraph/
├── FNode.h
└── (future: FCamera.h, FLight.h, etc.)

/Engine/Source/Runtime/Private/Renderer/SceneGraph/
├── FNode.cpp
└── (future: FCamera.cpp, etc.)
```

### Phase 4: Integration with Existing Scene3D System

1. **FScene3DLoader** should create `FNode` hierarchy with `FScene3DNode` data
2. **TestSceneGraph.cpp** should be updated to test new `FNode` functionality
3. **Build system** should include new SceneGraph directory

## Key Features to Implement

### 1. Dirty Flag Propagation
- When node transform changes, mark node and all children as dirty
- `UpdateWorldTransform()` recomputes only if dirty
- Parent dirty forces child re-evaluation

### 2. Template AddChild Method
```cpp
template<typename ChildType, typename... Args>
ChildType& FNode::AddChild(Args&&... args)
{
    auto child = std::make_unique<ChildType>(std::forward<Args>(args)...);
    child->Parent = shared_from_this();
    ChildType& ref = *child;
    Children.push_back(std::move(child));
    return ref;
}
```

### 3. Transform Hierarchy
- WorldTransform = Parent.WorldTransform * LocalTransform
- LocalTransform computed from Position, Rotation, Scale
- Support for Euler angles (convert to quaternion for matrix)

### 4. Camera Matrix Updates
- Override `UpdateWorldTransform()` in `FPerspectiveCamera`
- Compute view matrix from camera transform
- Compute projection matrix from FOV/aspect/near/far
- Update combined matrices

## Migration Steps

1. **Step 1**: Create FNode.h/.cpp with basic hierarchy
2. **Step 2**: Update FPerspectiveCamera inheritance and fix compilation
3. **Step 3**: Implement dirty flag and transform propagation
4. **Step 4**: Add template AddChild method
5. **Step 5**: Update tests to verify new functionality
6. **Step 6**: Build and test compilation

## Files to Create/Modify

### New Files:
1. `/Engine/Source/Runtime/Public/Renderer/SceneGraph/FNode.h`
2. `/Engine/Source/Runtime/Private/Renderer/SceneGraph/FNode.cpp`

### Modified Files:
1. `/Engine/Source/Runtime/Public/Renderer/Scene3D/FPerspectiveCamera.h` (change inheritance)
2. `/Engine/Source/Runtime/Private/Renderer/Scene3D/FPerspectiveCamera.cpp` (fix method calls)
3. `/Engine/Source/Runtime/Test/TestSceneGraph.cpp` (add FNode tests)

### CMake Updates:
- Add SceneGraph directory to CMakeLists.txt
- Ensure proper include paths

## Testing Strategy

1. **Unit Tests**:
   - Test parent-child hierarchy creation
   - Test dirty flag propagation
   - Test transform updates
   - Test camera matrix computation

2. **Integration Tests**:
   - Test with existing scene loading
   - Verify backward compatibility

3. **Build Verification**:
   - `./Build.sh --Config=Debug`
   - `./Build.sh --Config=Debug --Test`

## Success Criteria

1. ✅ New `FNode` class created in SceneGraph directory
2. ✅ `FNode` uses `std::shared_ptr<FScene3DNode>` as member (SceneData)
3. ✅ `FPerspectiveCamera` inherits from `FNode` and works correctly
4. ✅ Dirty flag propagation implemented
5. ✅ Parent-child hierarchy with transform updates
6. ✅ Template `AddChild` method
7. ✅ All tests pass
8. ✅ Build succeeds without errors

## Timeline Estimate

- **Phase 1 (FNode)**: 2-3 hours
- **Phase 2 (FPerspectiveCamera)**: 1-2 hours  
- **Phase 3 (Testing)**: 1-2 hours
- **Total**: 4-7 hours

## Risks and Mitigations

1. **Risk**: Breaking existing scene loading functionality
   - **Mitigation**: Keep `FScene3DNode` unchanged, use composition

2. **Risk**: Performance impact of dirty flag checks
   - **Mitigation**: Profile and optimize hot paths

3. **Risk**: Complex template method issues
   - **Mitigation**: Start with simple implementation, add complexity gradually

## Next Steps

1. Begin implementation of `FNode` class
2. Update `FPerspectiveCamera` inheritance
3. Test compilation incrementally
4. Add comprehensive unit tests
5. Document final architecture

---
*Plan created: 2026-03-22*
*Based on AI_task.md Goal 5 requirements*