# Scene Graph Test Plan

## Overview
Based on AI_task.md Goal 3: Write test cases for the scene graph system under `/Engine/Source/Runtime/Test`. The tests should use the sample asset `/Samples/Assets/sponza/Sponza01.gltf` and follow the patterns established in `TestAssetManager.cpp`.

## Current State Analysis

### Existing Scene Graph Implementation
From previous migration work (Goals 1 & 2), the following components exist:

1. **Mesh System**:
   - `IMesh.h` - Base interface for meshes
   - `StaticMesh.h` - Concrete mesh implementation with FP32 vertex format

2. **Material System**:
   - `IMaterial.h` - Base interface for materials
   - `PBRMaterial.h` - PBR material implementation

3. **Scene Graph**:
   - `Scene3DNode.h/.cpp` - Scene node hierarchy with mesh/material collections
   - `Scene3DLoader.h/.cpp` - Scene loading from Assimp (renamed from Scene3DManager)
   - `AssimpSceneObject.h/.cpp` - Assimp wrapper for file loading
   - `AssimpHelper.h` - Matrix conversion utilities

4. **Sample Asset**:
   - `/Samples/Assets/sponza/Sponza01.gltf` - 2816-line glTF file for testing

### Test Infrastructure Reference
`TestAssetManager.cpp` provides the following patterns:
- `DECLARE_LOG_CATEGORY(LogTest)` for logging
- `RECORD(test_name)` macro for test registration
- Helper functions for file operations
- Use of `HLVM_ENSURE()` for assertions
- Thread safety testing patterns
- Integration testing patterns

## Test Plan Structure

### Test File: `TestSceneGraph.cpp`

**Location**: `/Engine/Source/Runtime/Test/TestSceneGraph.cpp`

**Test Categories**:

1. **Basic Scene Loading Tests**
2. **Mesh Data Validation Tests**  
3. **Material System Tests**
4. **Scene Graph Hierarchy Tests**
5. **Performance & Memory Tests**
6. **Integration Tests**

### Detailed Test Cases

#### 1. Basic Scene Loading Tests

```cpp
RECORD(scene_load_basic)
{
    // Test basic scene loading from glTF file
    HLVM_LOG(LogTest, info, TXT("Testing basic scene loading"));
    
    const FPath scenePath = TXT("Samples/Assets/sponza/Sponza01.gltf");
    
    // Load scene using Scene3DLoader
    std::shared_ptr<FScene3DNode> scene = FScene3DLoader::LoadFromFile(scenePath);
    
    // Basic validation
    HLVM_ENSURE(scene != nullptr);
    HLVM_ENSURE(!scene->GetName().empty());
    HLVM_ENSURE(scene->GetAllMeshes().size() > 0);
    HLVM_ENSURE(scene->GetAllMaterials().size() > 0);
    
    HLVM_LOG(LogTest, info, TXT("Loaded scene: {} meshes, {} materials"), 
             scene->GetAllMeshes().size(), scene->GetAllMaterials().size());
}
```

#### 2. Mesh Data Validation Tests

```cpp
RECORD(mesh_data_validation)
{
    // Test mesh data extraction and format
    HLVM_LOG(LogTest, info, TXT("Testing mesh data validation"));
    
    const FPath scenePath = TXT("Samples/Assets/sponza/Sponza01.gltf");
    std::shared_ptr<FScene3DNode> scene = FScene3DLoader::LoadFromFile(scenePath);
    
    auto meshes = scene->GetAllMeshes();
    HLVM_ENSURE(!meshes.empty());
    
    // Validate first mesh
    auto firstMesh = meshes[0];
    HLVM_ENSURE(firstMesh != nullptr);
    
    // Check vertex format (FP32 position, normal, UV, tangent)
    // Each vertex should be 44 bytes (12+12+8+12)
    HLVM_ENSURE(firstMesh->GetVertexCount() > 0);
    HLVM_ENSURE(firstMesh->GetIndexCount() > 0);
    
    // Check bounding box
    auto bbox = firstMesh->GetBoundingBox();
    HLVM_ENSURE(bbox.IsValid());
    
    HLVM_LOG(LogTest, info, TXT("Mesh validation passed: {} vertices, {} indices"),
             firstMesh->GetVertexCount(), firstMesh->GetIndexCount());
}
```

#### 3. Material System Tests

```cpp
RECORD(material_system_validation)
{
    // Test PBR material extraction
    HLVM_LOG(LogTest, info, TXT("Testing material system"));
    
    const FPath scenePath = TXT("Samples/Assets/sponza/Sponza01.gltf");
    std::shared_ptr<FScene3DNode> scene = FScene3DLoader::LoadFromFile(scenePath);
    
    auto materials = scene->GetAllMaterials();
    HLVM_ENSURE(!materials.empty());
    
    // Validate first material
    auto firstMaterial = materials[0];
    HLVM_ENSURE(firstMaterial != nullptr);
    
    // Check material properties
    HLVM_ENSURE(!firstMaterial->GetName().empty());
    
    // PBR material should have texture slots
    // (Albedo, Normal, Metallic, Roughness, AO)
    HLVM_ENSURE(firstMaterial->GetTextureCount() >= 0);
    
    HLVM_LOG(LogTest, info, TXT("Material validation passed: {} materials loaded"),
             materials.size());
}
```

#### 4. Scene Graph Hierarchy Tests

```cpp
RECORD(scene_hierarchy_traversal)
{
    // Test scene graph traversal and hierarchy
    HLVM_LOG(LogTest, info, TXT("Testing scene hierarchy"));
    
    const FPath scenePath = TXT("Samples/Assets/sponza/Sponza01.gltf");
    std::shared_ptr<FScene3DNode> scene = FScene3DLoader::LoadFromFile(scenePath);
    
    // Test recursive traversal
    size_t totalMeshes = 0;
    size_t totalMaterials = 0;
    
    std::function<void(const FScene3DNode&)> countNodes = [&](const FScene3DNode& node) {
        totalMeshes += node.GetMeshes().size();
        totalMaterials += node.GetMaterials().size();
        
        for (const auto& child : node.GetChildren()) {
            countNodes(*child);
        }
    };
    
    countNodes(*scene);
    
    HLVM_ENSURE(totalMeshes > 0);
    HLVM_ENSURE(totalMaterials > 0);
    
    HLVM_LOG(LogTest, info, TXT("Scene hierarchy: {} total meshes, {} total materials"),
             totalMeshes, totalMaterials);
}
```

#### 5. Transform Accumulation Tests

```cpp
RECORD(transform_accumulation)
{
    // Test transform hierarchy accumulation
    HLVM_LOG(LogTest, info, TXT("Testing transform accumulation"));
    
    // Create a simple test scene with parent-child transforms
    // (This would require creating test scene programmatically)
    // For now, validate that loaded scene has valid transforms
    
    const FPath scenePath = TXT("Samples/Assets/sponza/Sponza01.gltf");
    std::shared_ptr<FScene3DNode> scene = FScene3DLoader::LoadFromFile(scenePath);
    
    // Check that root node has identity transform
    auto rootTransform = scene->GetTransform();
    HLVM_ENSURE(rootTransform == FMat4(1.0f));
    
    HLVM_LOG(LogTest, info, TXT("Transform validation passed"));
}
```

#### 6. File Format Support Tests

```cpp
RECORD(file_format_support)
{
    // Test different file formats (if available)
    HLVM_LOG(LogTest, info, TXT("Testing file format support"));
    
    // Note: Currently only testing glTF
    // Future tests could include FBX, OBJ, etc.
    
    const FPath gltfPath = TXT("Samples/Assets/sponza/Sponza01.gltf");
    
    // Test glTF loading
    std::shared_ptr<FScene3DNode> gltfScene = FScene3DLoader::LoadFromFile(gltfPath);
    HLVM_ENSURE(gltfScene != nullptr);
    
    HLVM_LOG(LogTest, info, TXT("glTF format supported"));
}
```

#### 7. Memory Management Tests

```cpp
RECORD(memory_management)
{
    // Test proper memory management and cleanup
    HLVM_LOG(LogTest, info, TXT("Testing memory management"));
    
    const FPath scenePath = TXT("Samples/Assets/sponza/Sponza01.gltf");
    
    // Load and unload multiple times
    for (int i = 0; i < 10; ++i) {
        std::shared_ptr<FScene3DNode> scene = FScene3DLoader::LoadFromFile(scenePath);
        HLVM_ENSURE(scene != nullptr);
        
        // Force cleanup by letting shared_ptr go out of scope
    }
    
    HLVM_LOG(LogTest, info, TXT("Memory management test passed - 10 load/unload cycles"));
}
```

#### 8. Error Handling Tests

```cpp
RECORD(error_handling)
{
    // Test error handling for invalid files
    HLVM_LOG(LogTest, info, TXT("Testing error handling"));
    
    // Test with non-existent file
    const FPath invalidPath = TXT("non_existent_file.gltf");
    
    bool exceptionCaught = false;
    try {
        std::shared_ptr<FScene3DNode> scene = FScene3DLoader::LoadFromFile(invalidPath);
    } catch (const std::exception& e) {
        exceptionCaught = true;
        HLVM_LOG(LogTest, info, TXT("Expected exception caught: {}"), e.what());
    }
    
    // Should throw or return nullptr
    HLVM_ENSURE(exceptionCaught);
    
    HLVM_LOG(LogTest, info, TXT("Error handling test passed"));
}
```

#### 9. Performance Benchmark Tests

```cpp
RECORD(performance_benchmark)
{
    // Benchmark scene loading performance
    HLVM_LOG(LogTest, info, TXT("Testing performance benchmark"));
    
    const FPath scenePath = TXT("Samples/Assets/sponza/Sponza01.gltf");
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Load scene multiple times for average
    constexpr int NUM_ITERATIONS = 5;
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::shared_ptr<FScene3DNode> scene = FScene3DLoader::LoadFromFile(scenePath);
        HLVM_ENSURE(scene != nullptr);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    double avgTime = duration.count() / static_cast<double>(NUM_ITERATIONS);
    
    HLVM_LOG(LogTest, info, TXT("Average load time: {} ms"), avgTime);
    
    // Performance threshold (adjust based on expectations)
    HLVM_ENSURE(avgTime < 5000.0); // Should load in under 5 seconds
}
```

#### 10. Integration with Asset Manager Tests

```cpp
RECORD(integration_with_asset_manager)
{
    // Test integration with existing AssetManager system
    HLVM_LOG(LogTest, info, TXT("Testing integration with AssetManager"));
    
    // This test would demonstrate how Scene3DLoader integrates with
    // the existing FAssetLoader/FResourceManager system
    
    // Example pattern from TestAssetManager.cpp:
    // 1. Use FAssetLoader for async scene loading
    // 2. Use FResourceManager<FScene3DNode> for resource management
    // 3. Test callback chains and promise fulfillment
    
    // Implementation would follow patterns from TestAssetManager.cpp
    // lines 569-630 (integration_both_modules test)
    
    HLVM_LOG(LogTest, info, TXT("Integration test placeholder - to be implemented"));
}
```

## Implementation Details

### Required Includes
```cpp
#include "Test.h"
#include "Renderer/Scene3D/Scene3DLoader.h"
#include "Renderer/Scene3D/Scene3DNode.h"
#include "Renderer/Mesh/StaticMesh.h"
#include "Renderer/Material/PBRMaterial.h"
#include "Platform/FileSystem/Path.h"
#include <chrono>
#include <functional>
```

### Test Helper Functions
Similar to `TestAssetManager.cpp`, we'll need helper functions for:
- Temporary file creation/deletion
- Memory measurement
- Performance timing
- Assertion logging

### Build Integration
The test file should be added to CMakeLists.txt in the test directory to be compiled and run with CTest.

## Dependencies

1. **Assimp Library**: Already integrated via `AssimpSceneObject.h`
2. **GLM**: For matrix operations (already used in `AssimpHelper.h`)
3. **HLVM Core**: FString, FPath, logging system
4. **Sample Assets**: `Sponza01.gltf` must be accessible at runtime

## Success Criteria

1. **Compilation**: All tests compile without errors
2. **Execution**: All tests pass when run with `./Build.sh --Config=Debug --Test`
3. **Coverage**: Tests cover all major scene graph components
4. **Performance**: Loading times are within acceptable limits
5. **Memory**: No memory leaks detected
6. **Error Handling**: Invalid inputs are handled gracefully

## Next Steps After Test Implementation

1. **Run Tests**: Execute `./Build.sh --Config=Debug --Target=TestSceneGraph --Test`
2. **Fix Issues**: Address any test failures or compilation errors
3. **Add to CI**: Integrate into GitHub Actions workflow
4. **Documentation**: Update project documentation with test coverage
5. **Expand Tests**: Add more edge cases and performance tests

## Notes

- Follow HLVM coding conventions (AGENTS.md)
- Use `FString` instead of `std::string`
- Use `FPath` instead of `fs::path`
- Use `HLVM_LOG` for test logging
- Use `HLVM_ENSURE` for assertions
- Maintain Public/Private separation
- Ensure thread safety where applicable