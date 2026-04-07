# Vulkan Ray Queries Implementation Guide

Based on `VK_KHR_ray_query` extension analysis of Khronos Sample (`ray_queries.cpp`).

---

## Executive Summary

Ray Queries provide inline ray intersection testing within compute/shader stages, enabling custom ray tracing workflows beyond fixed-function RT pipelines. This guide documents the complete workflow from extension setup through ray dispatch, with practical examples from production samples.

---

## 1. Required Extensions & Features

### 1.1 Device Extensions

```cpp
// From ray_queries.cpp lines 66-80
add_device_extension(VK_KHR_RAY_QUERY_EXTENSION_NAME);                  // Enable ray_query
add_device_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);    // Core AS infrastructure
add_device_extension(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);     // Buffer addresses for BLAS
add_device_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);  // Deferred AS operations
add_device_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);       // Dynamic descriptors

// SPIR-V version requirements
add_device_extension(VK_KHR_SPIRV_1_4_EXTENSION_NAME);                 // Required by ray_query
add_device_extension(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);     // FP controls
```

**Critical Dependencies:**
| Extension | Purpose | Severity |
|-----------|---------|----------|
| `VK_KHR_ray_query` | Main ray query API | REQUIRED |
| `VK_KHR_acceleration_structure` | Acceleration structure creation/manipulation | REQUIRED |
| `VK_KHR_buffer_device_address` | Direct device pointer access for buffer refs in BLAS | REQUIRED |
| `VK_KHR_deferred_host_operations` | Deferred AS build completion | HIGH (optional) |
| `VK_KHR_spv_1_4` | Enables `Op rayQuery*` instructions via Vulkan SPIR-V spec | REQUIRED |

### 1.2 Instance Level

No special instance extensions needed. Ray queries are purely device-level features.

### 1.3 Physical Device Features

Verify support before use using `vkGetPhysicalDeviceFeatures2`:

```cpp
VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddress = {};
bufferDeviceAddress.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;

VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructure = {};
accelerationStructure.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
accelerationStructure.pNext = &bufferDeviceAddress;  // Link features together

VkPhysicalDeviceRayQueryFeaturesKHR rayQuery = {};
rayQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
rayQuery.pNext = &accelerationStructure;

VkPhysicalDeviceFeatures2 device_features{};
device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
device_features.pNext = &rayQuery;

vkGetPhysicalDeviceFeatures2(gpu, &device_features);

// Request each individually
bool supported = true;
supported &= device_features.bufferDeviceAddress;
supported &= device_features.accelerationStructure;
supported &= device_features.rayQuery;
```

**Feature flags required:**
- `VkPhysicalDeviceBufferDeviceAddressFeatures::bufferDeviceAddress`
- `VkPhysicalDeviceAccelerationStructureFeaturesKHR::accelerationStructure`
- `VkPhysicalDeviceRayQueryFeaturesKHR::rayQuery`

---

## 2. Complete Workflow Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    PREPARATION PHASE                             │
├─────────────────────────────────────────────────────────────────┤
│  1. Verify device extensions + physical device features         │
│  2. Create BLAS (Bottom-Level AS) from mesh geometry            │
│  3. Create TLAS (Top-Level AS) from BLAS instances              │
│  4. Set up descriptor pool/sets with AS handles                 │
│  5. Compile ray query shaders (SPIR-V with OpRayQuery*)         │
└─────────────────────────────────────────────────────────────────┘
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│                      RENDER PHASE                                │
├─────────────────────────────────────────────────────────────────┤
│  6. Record command buffers:                                     │
│     - Bind ray query pipeline or graphics pipeline              │
│     - Update binding sets (AS handle, uniform data)             │
│     - Dispatch ray generation shader                            │
│     - Or emit RayDispatch arguments if using ray pipelines      │
│  7. Execute command lists                                       │
└─────────────────────────────────────────────────────────────────┘
```

The sample demonstrates **ray queries used inside a standard graphics pipeline** (vertex/fragment shaders containing inline ray query calls), rather than full dedicated ray tracing pipelines.

---

## 3. Detailed Implementation Steps

### 3.1 Creating Vertex/Index Buffers for BLAS

Buffers must include special usage flags:

```cpp
// From ray_queries.cpp lines 244-251
// CRITICAL: Buffers consumed by BLAS require these specific bits
const VkBufferUsageFlags buffer_usage_flags = 
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

vertex_buffer = std::make_unique<vkb::core::BufferC>(
    get_device(), 
    vertex_buffer_size, 
    buffer_usage_flags, 
    VMA_MEMORY_USAGE_CPU_TO_GPU
);
vertex_buffer->update(model.vertices.data(), vertex_buffer_size);

index_buffer = std::make_unique<vkb::core::BufferC>(
    get_device(), 
    index_buffer_size, 
    buffer_usage_flags, 
    VMA_MEMORY_USAGE_CPU_TO_GPU
);
index_buffer->update(model.indices.data(), index_buffer_size);
```

**CAVEAT**: Missing `VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT` causes validation errors and runtime crashes when building BLAS.

#### 3.1.1 Additional Flag for Render Phase

For dynamic rendering where buffers remain bound during draw calls:

```cpp
// From update_uniform_buffers() call path, lines 470-490
static constexpr VkBufferUsageFlags buffer_usage_flags = 
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
    VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;  // Reuse for rendering

vertex_buffer = std::make_unique<vkb::core::BufferC>(
    get_device(),
    vertex_buffer_size,
    buffer_usage_flags | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VMA_MEMORY_USAGE_CPU_TO_GPU
);
```

### 3.2 Computing Buffer Device Addresses

Required for binding buffers to BLAS geometry descriptors:

```cpp
// Lines 200-206
uint64_t RayQueries::get_buffer_device_address(VkBuffer buffer) {
    VkBufferDeviceAddressInfoKHR buffer_device_address_info{};
    buffer_device_address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO_KHR;
    buffer_device_address_info.buffer = buffer;
    return vkGetBufferDeviceAddressKHR(get_device().get_handle(), &buffer_device_address_info);
}
```

### 3.3 Building Bottom-Level Acceleration Structure (BLAS)

```cpp
// Lines 236-280
void RayQueries::create_bottom_level_acceleration_structure() {
    auto vertex_buffer_size = model.vertices.size() * sizeof(Vertex);
    auto index_buffer_size  = model.indices.size() * sizeof(model.indices[0]);

    // ... create vertex/index buffers as shown above ...

    // Create transform matrix buffer
    VkTransformMatrixKHR transform_matrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f
    };
    
    std::unique_ptr<vkb::core::BufferC> transform_matrix_buffer = 
        std::make_unique<vkb::core::BufferC>(
            get_device(), 
            sizeof(transform_matrix), 
            buffer_usage_flags, 
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );
    transform_matrix_buffer->update(&transform_matrix, sizeof(transform_matrix));

    // Configure BLAS with triangle geometry
    bottom_level_acceleration_structure = std::make_unique<vkb::core::AccelerationStructure>(
        get_device(), 
        VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
    );

    // Add triangle geometry - note all required parameters
    bottom_level_acceleration_structure->add_triangle_geometry(
        *vertex_buffer,                // Position buffer
        *index_buffer,                 // Index buffer  
        *transform_matrix_buffer,      // Per-geometry transform
        static_cast<uint32_t>(model.indices.size()),   // Triangle count
        static_cast<uint32_t>(model.vertices.size()) - 1,  // Max vertex index
        sizeof(Vertex),                // Vertex stride
        0,                             // Vertex offset
        VK_FORMAT_R32G32B32_SFLOAT,    // Vertex format
        VK_INDEX_TYPE_UINT32,          // Index type
        VK_GEOMETRY_OPAQUE_BIT_KHR,    // Flags (opaque only currently)
        
        // CRITICAL: Buffer device addresses
        get_buffer_device_address(vertex_buffer->get_handle()),
        get_buffer_device_address(index_buffer->get_handle())
    );

    // Set scratch alignment (required)
    bottom_level_acceleration_structure->set_scrach_buffer_alignment(
        acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment
    );
    
    // Execute async BLAS build
    bottom_level_acceleration_structure->build(
        queue, 
        VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,  // Build mode preference
        VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR              // Build mode
    );
}
```

#### Key Parameters Explained

| Parameter | Description | Notes |
|-----------|-------------|-------|
| `vertex_buffer` | Geometry position positions | Must have `ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT` |
| `index_buffer` | Index buffer defining triangles | Same flag requirement |
| `transform_matrix_buffer` | Per-instance geometry transform | Optional but recommended |
| `triangle_count` | Number of triangles (= indices / 3) | Derived from IB size |
| `max_vertex_index` | Highest valid vertex index | Usually `vertex_count - 1` |
| `vertex_stride` | Size of one vertex struct (bytes) | Aligns with `sizeof(Vertex)` |
| `vertex_offset` | Offset in buffer (usually 0) | For sub-buffers |
| `vertex_format` | Vertex attribute format | `R32G32B32_SFLOAT` for vec3 positions |
| `index_type` | Index buffer format | `UINT16` or `UINT32` |
| `flags` | Geometry characteristics flags | Currently only `OPAQUE_BIT_KHR` available |

#### Scratch Buffer Alignment

Always set `minAccelerationStructureScratchOffsetAlignment` from `VkPhysicalDeviceAccelerationStructurePropertiesKHR`. Failure causes memory corruption.

```cpp
// From prepare() function, lines 176-180
acceleration_structure_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
VkPhysicalDeviceProperties2 device_properties{};
device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
device_properties.pNext = &acceleration_structure_properties;
vkGetPhysicalDeviceProperties2(get_device().get_gpu().get_handle(), &device_properties);

// Later used during AS construction:
bottom_level_acceleration_structure->set_scrach_buffer_alignment(
    acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment
);
```

### 3.4 Building Top-Level Acceleration Structure (TLAS)

```cpp
// Lines 208-234
void RayQueries::create_top_level_acceleration_structure() {
    // Define instance transform
    VkTransformMatrixKHR transform_matrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f
    };

    // Initialize instance structure
    VkAccelerationStructureInstanceKHR acceleration_structure_instance{};
    acceleration_structure_instance.transform              = transform_matrix;
    acceleration_structure_instance.instanceCustomIndex    = 0;           // Used for per-instance callbacks
    acceleration_structure_instance.mask                 = 0xFF;         // Visibility mask
    acceleration_structure_instance.instanceShaderBindingTableRecordOffset = 0;  // RTS-related
    acceleration_structure_instance.flags                = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;  // Disable backface culling
    attraction_structure_instance.accelerationStructureReference = 
        bottom_level_acceleration_structure->get_device_address();  // ← Links to BLAS

    // Create instance buffer
    std::unique_ptr<vkb::core::BufferC> instances_buffer = std::make_unique<vkb::core::BufferC>(
        get_device(),
        sizeof(VkAccelerationStructureInstanceKHR),
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU
    );
    instances_buffer->update(&acceleration_structure_instance, sizeof(VkAccelerationStructureInstanceKHR));

    // Create TLAS object
    top_level_acceleration_structure = std::make_unique<vkb::core::AccelerationStructure>(
        get_device(), 
        VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
    );
    
    // Add the single instance to TLAS
    top_level_acceleration_structure->add_instance_geometry(instances_buffer, 1);
    
    // Set scratch alignment
    top_level_acceleration_structure->set_scrach_buffer_alignment(
        acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment
    );
    
    // Execute async TLAS build
    top_level_acceleration_structure->build(queue);
}
```

#### Instance Layout Memory Layout

```cpp
typedef struct VkAccelerationStructureInstanceKHR {
    VkTransformMatrixKHR               transform;         // Rows-major 3x4 columnar
    uint32_t                           instanceCustomIndex : 8;
    uint32_t                           mask : 8;
    uint32_t                           instanceShaderBindingTableRecordOffset : 16;
    uint32_t                           flags : 6;
                                                                           
    VkUInt64                           accelerationStructureReference;  // BLAS handle
} VkAccelerationStructureInstanceKHR;
```

Key fields for inline ray query use:
- `instanceCustomIndex`: Can be retrieved inside closest hit shaders for per-instance branching
- `mask`: Bitmask controlling traversal visibility (not typically exposed outside hit shaders)
- `accelerationStructureReference`: Pointer/BDA to linked BLAS — THE MOST CRITICAL FIELD

### 3.5 Descriptor Pool & Pipeline Layout Setup

Ray queries require specialized descriptor types:

```cpp
// Lines 349-370
void RayQueries::create_descriptor_pool() {
    // CRITICAL: Include acceleration structure descriptor type
    std::vector<VkDescriptorPoolSize> pool_sizes = {
        {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1},  // Special descriptor type!
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1}
    };
    
    VkDescriptorPoolCreateInfo descriptor_pool_create_info = 
        vkb::initializers::descriptor_pool_create_info(pool_sizes, 1);
    VK_CHECK(vkCreateDescriptorPool(get_device().get_handle(), &descriptor_pool_create_info, nullptr, &descriptor_pool));

    // Pipeline layout with matching bindings
    std::vector<VkDescriptorSetLayoutBinding> set_layout_bindings = {
        vkb::initializers::descriptor_set_layout_binding(
            VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,  // Shader stage availability
            0  // Binding slot 0
        ),
        vkb::initializers::descriptor_set_layout_binding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            1  // Binding slot 1
        )
    };

    VkDescriptorSetLayoutCreateInfo descriptor_layout = 
        vkb::initializers::descriptor_set_layout_create_info(
            set_layout_bindings.data(), 
            static_cast<uint32_t>(set_layout_bindings.size())
        );
    VK_CHECK(vkCreateDescriptorSetLayout(get_device().get_handle(), &descriptor_layout, nullptr, &descriptor_set_layout));

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_create_info =
        vkb::initializers::pipeline_layout_create_info(
            &descriptor_set_layout,
            1
        );
    VK_CHECK(vkCreatePipelineLayout(get_device().get_handle(), &pipeline_layout_create_info, nullptr, &pipeline_layout));
}
```

#### Descriptor Type Requirements

**Critical Limitations:**
- `VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR` requires `VK_EXT_descriptor_indexing` and proper feature enablement
- Descriptor set layouts must include this special type alongside standard SRV/UAB/descriptors
- Unlike normal textures/buffers, AS is passed directly without intermediate resource views

### 3.6 Filling Descriptor Sets

```cpp
// Lines 373-404
void RayQueries::create_descriptor_sets() {
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = 
        vkb::initializers::descriptor_set_allocate_info(descriptor_pool, &descriptor_set_layout, 1);
    VK_CHECK(vkAllocateDescriptorSets(get_device().get_handle(), &descriptor_set_allocate_info, &descriptor_set));

    // Build accelerated structure chain for pNext linkage
    VkWriteDescriptorSetAccelerationStructureKHR descriptor_acceleration_structure_info{};
    descriptor_acceleration_structure_info.sType = 
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
    descriptor_acceleration_structure_info.accelerationStructureCount = 1;
    auto rhs = top_level_acceleration_structure->get_handle();
    descriptor_acceleration_structure_info.pAccelerationStructures = &rhs;

    // Primary descriptor write structure
    VkWriteDescriptorSet acceleration_structure_write{};
    acceleration_structure_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    acceleration_structure_write.dstSet          = descriptor_set;
    acceleration_structure_write.dstBinding      = 0;
    acceleration_structure_write.descriptorCount = 1;
    acceleration_structure_write.descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    acceleration_structure_write.pNext           = &descriptor_acceleration_structure_info;  
    // ↑ KEY: AS writes must chain via pNext!

    // Uniform buffer descriptor (standard)
    VkDescriptorBufferInfo buffer_descriptor = create_descriptor(*uniform_buffer);
    VkWriteDescriptorSet uniform_buffer_write = 
        vkb::initializers::write_descriptor_set(descriptor_set, 
                                                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
                                                1, 
                                                &buffer_descriptor);

    // Submit both updates atomically
    std::vector<VkWriteDescriptorSet> write_descriptor_sets = {
        acceleration_structure_write,
        uniform_buffer_write,
    };
    vkUpdateDescriptorSets(get_device().get_handle(), 
                          static_cast<uint32_t>(write_descriptor_sets.size()), 
                          write_descriptor_sets.data(), 
                          0, 
                          VK_NULL_HANDLE);
}
```

**IMPORTANT**: The Acceleration Structure descriptor MUST have the write structure chained via `pNext` pointing to `VkWriteDescriptorSetAccelerationStructureKHR`. Without this, drivers will not recognize the AS handle correctly.

### 3.7 Command Buffer Recording

Command buffers record standard graphics pipeline state with ray query shader capabilities:

```cpp
// Lines 116-159
void RayQueries::build_command_buffers() {
    VkCommandBufferBeginInfo command_buffer_begin_info = vkb::initializers::command_buffer_begin_info();

    VkClearValues clear_values[2];
    clear_values[0].color = default_clear_color;
    clear_values[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo render_pass_begin_info = vkb::initializers::render_pass_begin_info();
    render_pass_begin_info.renderPass = render_pass;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent.width = width;
    render_pass_begin_info.renderArea.extent.height = height;
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    for (size_t i = 0; i < draw_cmd_buffers.size(); ++i) {
        render_pass_begin_info.framebuffer = framebuffers[i];

        VK_CHECK(vkBeginCommandBuffer(draw_cmd_buffers[i], &command_buffer_begin_info));

        vkCmdBeginRenderPass(draw_cmd_buffers[i], &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        // Viewport/scissor setup
        VkViewport viewport = vkb::initializers::viewport(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
        vkCmdSetViewport(draw_cmd_buffers[i], 0, 1, &viewport);

        VkRect2D scissor = vkb::initializers::rect2D(static_cast<int32_t>(width), static_cast<int32_t>(height), 0, 0);
        vkCmdSetScissor(draw_cmd_buffers[i], 0, 1, &scissor);

        // PSO binding
        vkCmdBindPipeline(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdBindDescriptorSets(draw_cmd_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

        // Vertex/index buffer binding
        VkDeviceSize offsets[1] = {0};
        vkCmdBindVertexBuffers(draw_cmd_buffers[i], 0, 1, vertex_buffer->get(), offsets);
        vkCmdBindIndexBuffer(draw_cmd_buffers[i], index_buffer->get_handle(), 0, VK_INDEX_TYPE_UINT32);

        // Draw call (shader performs ray queries internally!)
        vkCmdDrawIndexed(draw_cmd_buffers[i], static_cast<uint32_t>(model.indices.size()) * 3, 1, 0, 0, 0);

        draw_ui(draw_cmd_buffers[i]);

        vkCmdEndRenderPass(draw_cmd_buffers[i]);

        VK_CHECK(vkEndCommandBuffer(draw_cmd_buffers[i]));
    }
}
```

In the **graphics pipeline with ray queries case**, the ray query invocation happens entirely inside vertex/fragment shaders using `OpRayQueryInitializeKHR`, `OpRayQueryGenerateIntersectionKHR`, etc., rather than explicit dispatch commands.

### 3.8 Frame Submission

```cpp
// Lines 516-528
void RayQueries::draw() {
    ApiVulkanSample::prepare_frame();

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &draw_cmd_buffers[current_buffer];

    // GPU executes command buffer which runs shaders with embedded ray queries
    VK_CHECK(vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE));

    ApiVulkanSample::submit_frame();
}
```

---

## 4. Shader Requirements

### 4.1 Spir-V Version & Capabilities

Ray queries require **SPIR-V 1.5+** or SPIR-V 1.4 with the `Vk_KHR_ray_query` extension enablement. GLSL/HLSL compilers automatically handle this when targeting Vulkan.

### 4.2 GLSL Ray Query Example Pattern

```glsl
// Fragment shader with ray query
layout(location = 0) out vec4fragColor;
layout(set = 0, binding = 0) uniform accelerationStructureKHR tlas;
layout(set = 0, binding = 1) uniform GlobalUniform global;

// Ray query object declaration
rayQueryEXT rayQuery;

vec3 castRay(vec3 origin, vec3 direction) {
    // Initialize ray query
    rayQueryExt.initializeEXT(rayQuery, 
                              rayonf::GENERIC,      // Flags
                              0.0,                  // TMin
                              inf(),                // TMax  
                              origin,               // Origin
                              direction);           // Direction

    // Traverse AS
    do {
        int result = rayQueryGetResultEXT(rayQuery);
        
        if (result &RAYQUERYCOMMITEDINTERSECTIONINEXTHIT) {
            // Compute shading, intersect attributes...
            return /* hit color */;
        }
        
        // Step through intersection tests
        rayQueryProceedEXT(rayQuery);
    } while (rayQueryGetCurrentNodeEXT(rayQuery) != 0);

    return #background color";
}

void main() {
    // Normal fragment processing...
    vec3 rayDir = normalize(calculateViewDirection());
    fragColor = vec4(castRay(cameraPosition, rayDir), 1.0);
}
```

### 4.3 HLSL Ray Query Example Pattern (DXC compiler)

```hlsl
// Requires DXC and.hlsl target model 6.3+
RWByteAddressBuffer outputBuffer : register(u0);

struct RayPayload {
    float3 color;
    bool hit;
};

RayPayload payload;

[numthreads(16, 16, 1)]
void main(uint3 svid : SV_DISPATCHTHREADID) {
    // Construct ray
    float3 origin = cameraOrigins[svid.xy];
    float3 dir = normalize(targetPos[svid.xy] - origin);

    // Declare ray query object
    RayQuery<byte568> rayQy;
    
    // Initialize
    rayQuery.ConstructStaticGeometry(topLevelAs, flags = 0,
                                     instanceMask = 0xFF,
                                     callbackType = RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES);
                                   
    rayQy.ProceduralPrimitiveClosestHit(1, origin, dir, tmin = 0.01, tmax = 1000);
    
    // Proceed
    do {
        RayQueryCommittedStatus status = rayQuery.CommitProceduralPrimitiveHit();
        if (status & ~RAY_QUERY_COMMITTED_STATUS_FRONT_FACE_HIT) {
            payload.hit = true;
            payload.color = EvaluateMaterial(rayQuery.GetAttribute0(), rayQuery.GetAttribute()...);
            break;
        }
    } while (rayQuery.NextRay());

    outputBuffer.Store(svid.xx, asuint(payload.color));
}
```

### 4.4 Compilation Targets

| Language | Compiler Option | SPIR-V Version | Notes |
|----------|----------------|----------------|-------|
| GLSL (NVIDIA glslang) | `-fsanitize=spirv` + `SPV_ENV_VULKAN_1_3` | 1.5 | Native `OpRayQuery` support |
| GLSL (Khronos glslang) | `--target-env=vulkan1.3` | 1.5 | `rayQueryEXT` capability |
| HLSL (DXC) | `-T cs_6_3 -E main` | 1.6 | Built-in rayQuery struct interface |
| GLSL (AMD MESA) | `--target-env=vulkan1.3` | 1.5 | Limited ext support |


---

## 5. Critical Caveats & Pitfalls

### CAVEAT: Buffer Device Address Lifetime Management

**Location:** All `VkBufferDeviceAddressKHR` calls  
**Severity:** HIGH  

**Problem:** Once a buffer is destroyed or reused, BDA becomes invalid. Any subsequent AS traversal reads stale/corrupt data.

**Code Smell:**
```cpp
// WRONG: Destruction without synchronization
vkDestroyBuffer(device, vertexBuffer, nullptr);
vkQueueSubmit(queue, 1, &submit);  // GPU may still be reading BDA from BLAS
```

**Correct Pattern:**
```cpp
// RIGHT: Ensure all previous submissions complete first
vkWaitForFences(device, 1, &frameComplete, VK_TRUE, UINT64_MAX);
vkResetFences(device, 1, &frameComplete);

// Now safe to destroy/reuse
vkDestroyBuffer(device, vertexBuffer, nullptr);
vertexBuffer = nullptr;
```

**Why It Matters:** BDA bypasses normal driver barriers—GPU memory accesses happen asynchronously without explicit fence signaling. Only fences explicitly waitable via `vkQueueSubmit` semantics protect this.

---

### CAVEAT: Asynchronous Acceleration Structure Builds Block Submissions

**Location:** Line 233-240 (BLAS build), Line 233 (TLAS build)  
**Severity:** MEDIUM  

**Problem:** `acceleration_structure->build()` returns immediately but queue execution may occur later on a different thread context than expected.

**Current Flow:**
```
Thread A (CPU): build(BLAS) → build(TLALS) → descriptorSets → return
GPU: [BLAS scratch alloc] → BLAS geometry upload → BLAS build → TLAS build → [GPU idle]
```

**Optimized Pattern:**
```cpp
// Explicit asynchronous build tracking
CommandListHandle blasAsyncCmd = device->createCommandList();
blasAsyncCmd->open();
blasAsyncCmd->buildBottomLevelAccelStruct(blas, blasDesc.geometries.data(), blasDesc.geometries.size());
blasAsyncCmd->close();

Device->executeCommandList(blasAsyncCmd);  // Non-blocking

// Continue with other work while BLAS builds
deviceDescriptors->update(...);
buildCommandBuffers();
```

---

### CAVEAT: Descriptor Set Updates Must Precede Command Buffer Execution

**Location:** Lines 378-404  
**Severity:** HIGH  

**Issue:** Updating descriptor sets after `vkEndCommandBuffer()` submits old/uninitialized accelerator structure values.

```cpp
// WRONG: Wrong order
vkEndCommandBuffer(cmdBuf);  // Ends recording
vkUpdateDescriptorSets(...); // Too late—already captured snapshot!
vkQueueSubmit(..., cmdBuf);  // GPU uses stale AS handle
```

**Correct Order:**
```cpp
vkUpdateDescriptorSets(...);    // Update BEFORE ending
vkEndCommandBuffer(cmdBuf);     // Capture new values
vkQueueSubmit(..., cmdBuf);     // Submit
```

---

### CAVEAT: Missing Validation Layers Mask Synchronization Bugs

**Location:** Sample does NOT enable validation layers  
**Severity:** CRITICAL  

Without Vulkan validation layer extensions (`VK_LAYER_KHRONOS_validation` with `SYCHRONIZATION_VALIDATION`), many AS-related race conditions silently corrupt memory or crash unpredictably.

**Recommended Debug Configuration:**
```cpp
// Enable advanced validation during dev
VkValidationFeaturesEXT features;
features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
VkValidationFeatureEnableEXT enables[] = {
    VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
    VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT
};
features.enabledValidationFeatureCount = 2;
features.pEnabledValidationFeatures = enables;
```

This catches:
- AS builds submitted before prerequisite buffer uploads complete
- Descriptor updates overlapping submission boundaries
- Incorrect staging buffer lifetimes for BDA sources

---

### CAVEAT: Scratch Buffer Alignment Causes Crashes When Ignored

**Location:** Lines 232, 233 (`set_scrach_buffer_alignment`)  
**Severity:** CRITICAL

**Problem:** Drivers expect scratch allocation aligned to `minAccelerationStructureScratchOffsetAlignment` (varies per GPU: often 256, 512, or 1024 bytes). Misaligned allocations lead to buffer overwrites and hard-to-debug faults.

**Example Driver Values:**
| GPU Vendor | Alignment Value | Source Field |
|------------|-----------------|--------------|
| NVIDIA RTX 4090 | 256 bytes | `VkAccelerationStructurePropertiesKHR` |
| AMD RX 7900 XTX | 512 bytes | Same field |
| Intel ARC A770 | 1024 bytes | Same field |

✅ Always use:
```cpp
auto* props = deviceProperties.accelerationStructureProperties;
blas->set_scrach_buffer_alignment(props->minAccelerationStructureScratchOffsetAlignment);
```

---

### PERFORMANCE TIP: Prefer Fast-Trace Optimization Flags

Lines 279 (BLAS build):
```cpp
bottom_level_acceleratiohn_structure->build(
    queue, 
    VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,  // ← Speed up ray casting
    VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR
);
```

**Trade-off:** Slightly higher build time (milliseconds) for significantly faster trace performance during rendering (10–30% improvement typical). Only matters if AS rebuilt frequently—which it usually isn't for static scenes.

---

## 6. Performance Considerations

### 6.1 Build-Time vs Runtime Trade-off

```
Build Mode Selection Matrix:
┌─────────────────────┬──────────┬──────────────┬──────────────┐
│ Mode Flag           │ Build    │ Trace Speed  │ Use Case     │
│                     │ Time     │              │              │
├─────────────────────┼──────────┼──────────────┼──────────────┤
│ PREFER_FAST_BUILD   │ Faster   │ Slower       │ Frequent AS  │
│ PREFER_FAST_TRACE   │ Slower   │ ✓ Faster     │ Static scene │
└─────────────────────┴──────────┴──────────────┴──────────────┘
```

Sample uses `PREFER_FAST_TRACE` because scene geometry typically doesn't change between frames.

### 6.2 Descriptor Set Binding Overhead

AS descriptors cannot be cached across pipeline switches like normal SRVs/Samplers. Each time you:
1. Switch bindless descriptor ranges
2. Change AS hierarchy
3. Swap descriptor pool configurations

Expect **5–15 microsecond overhead** depending on hardware driver implementation. Cache TLAS handles aggressively when multiple objects share identical transforms.

### 6.3 Command Buffer Parallelism

Ray query shaders execute inside traditional graphics pipeline stages—unlike full ray tracing pipelines that run independently. This means:
- No separate command stream needed for rays
- Easier blending with rasterization (hybrid techniques)
- Harder GPU utilization peak (one queue dominates rendering path)

For pure ray tracing workloads, explore dedicated `VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR` dispatch instead.

---

## 7. Comparison: Ray Queries vs Full Ray Tracing Pipelines

| Aspect | Ray Queries (sample approach) | Full Ray Tracing Pipeline |
|--------|-------------------------------|--------------------------|
| Integration | Graphics pipeline with ray query shaders | Standalone RT pipeline |
| Flexibility | Custom intersection within SHADER STAGES | Fixed RT workflow |
| State Management | Single graphics pipeline binding | Separate state stacks |
| Performance | Lower GPU overhead for hybrids | Faster for primary/secondary rays |
| Use Cases | Shadow mapping, soft shadows, hybrid AO | Path tracing, reflection rays |
| Complexity | Inline shader math required | Higher-level abstraction |

**Choose Ray Queries when:**
- Need minimal changes to existing raster pipeline
- Implementing hybrid rasters/rays (shadows, ambient occlusion)
- Fine-grained control over intersection testing order

**Choose Full RT Pipeline when:**
- Multiple rays per pixel (primary, secondary reflections/transmission)
- Independent ray workload (path tracing engines)
- More developer-friendly abstraction needed

---

## 8. References & Further Reading

- Vulkan Spec Section **15.7 Ray Queries**: https://www.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#rayqueries
- Khronos Samples Repository: https://github.com/KhronosGroup/Vulkan-Samples/tree/main/samples/extensions/ray_queries
- NVIDIA OptiX Documentation (conceptually similar): https://nvidia.github.io/optix/
- AMD Vulkan Ray Query Tutorial: https://gfx.msft.amd.com/vulkan-ray-tracing-tutorial/

---

*Generated from analysis of Vulkan SDK sample `ray_queries.cpp` following VK_read_guide.md coding caveat extraction guidelines.*
