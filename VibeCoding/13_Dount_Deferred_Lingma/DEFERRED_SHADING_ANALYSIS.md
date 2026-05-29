# Deferred Shading Test 分析与实现计划

## 当前状态分析

### ✅ 已完成的基础设施

HLVM Engine 已经有**完整的 Deferred Rendering 架构**:

#### 1. G-Buffer 系统
- **FGBufferTextures**: G-Buffer 纹理管理 (Albedo, Normal, Depth, etc.)
- **FGBufferFillPass**: Geometry Pass - 渲染场景几何到 G-Buffer
- **FViewConstants**: View/Projection 常量缓冲

#### 2. Lighting 系统
- **FDeferredLightingPass**: Lighting Pass - 从 G-Buffer 读取并计算光照
- **FLightingConstants**: 光照常量 (lights, materials, etc.)

#### 3. 后处理
- **FDeferredBlitPass**: Blit Pass - 将结果输出到屏幕

### ❌ 当前 TestDeferredShading.cpp 的问题

#### 问题 1: Shader Blob 格式错误 (未修复)

**第 259, 268 行**:
```cpp
auto VSBlobData = ReadBinaryFile(FPath::Combine(DataDir, TXT("blit_vs.sblob")).string());  // ❌
auto FSBlobData = ReadBinaryFile(FPath::Combine(DataDir, TXT("blit_ps.sblob")).string());  // ❌
```

**应该改为**:
```cpp
auto VSBlobData = ReadBinaryFile(FPath::Combine(DataDir, TXT("blit_vs.bin")).string());  // ✅
auto FSBlobData = ReadBinaryFile(FPath::Combine(DataDir, TXT("blit_ps.bin")).string());  // ✅
```

#### 问题 2: Binding Slot 不匹配

**第 319 行** (Binding Layout):
```cpp
LayoutDesc.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(256));  // ❌ 应该是 0
```

**第 409 行** (Binding Set):
```cpp
SetDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(256, UniformBuffer));  // ❌ 应该是 0
```

**原因**: 
- HLSL shader 使用 `register(b0)`
- ShaderMake 编译时使用 `--bRegShift 0`
- SPIR-V binding = 0
- C++ 应该用 slot 0,不是 256!

#### 问题 3: Far Plane 太小

**第 357 行**:
```cpp
Context.Camera->SetFarPlane(10.0f);  // ❌ 可能裁剪旋转的立方体
```

**建议改为**:
```cpp
Context.Camera->SetFarPlane(100.0f);  // ✅ 安全值
```

#### 问题 4: **这不是 Deferred Shading!**

**当前渲染流程** (简化版):
```cpp
// Forward Rendering (当前)
Clear Color + Depth
↓
Draw Cube Directly to Framebuffer
↓
Present
```

**真正的 Deferred Shading 应该是**:
```cpp
// Phase 1: Geometry Pass (G-Buffer Fill)
Clear G-Buffer textures
↓
Render geometry to G-Buffer:
  - Albedo → GBuffer.Albedo
  - Normal → GBuffer.Normal
  - Depth  → GBuffer.Depth
  - etc.

// Phase 2: Lighting Pass
Read from G-Buffer textures
↓
Compute lighting per-pixel
↓
Write shaded color to Output

// Phase 3: Blit Pass
Blit Output to Screen
↓
Present
```

---

## 完整的 Deferred Shading 实现计划

### Phase 1: 修复当前测试 (立即)

**目标**: 让当前的简单 cube 测试正常工作

**修改清单**:

#### 修改 1: 更新 Shader 文件扩展名
```cpp
// Line 259
auto VSBlobData = ReadBinaryFile(FPath::Combine(DataDir, TXT("blit_vs.bin")).string());

// Line 268  
auto FSBlobData = ReadBinaryFile(FPath::Combine(DataDir, TXT("blit_ps.bin")).string());
```

#### 修改 2: 修正 Binding Slot
```cpp
// Line 319
LayoutDesc.addItem(nvrhi::BindingLayoutItem::ConstantBuffer(0));  // Changed from 256

// Line 409
SetDesc.addItem(nvrhi::BindingSetItem::ConstantBuffer(0, UniformBuffer));  // Changed from 256
```

#### 修改 3: 增加 Far Plane
```cpp
// Line 357
Context.Camera->SetFarPlane(100.0f);  // Changed from 10.0f
```

**预期结果**: 旋转的彩色立方体可见

---

### Phase 2: 实现真正的 Deferred Shading (短期)

**目标**: 使用现有的 FGBufferFillPass 和 FDeferredLightingPass

#### Step 1: 准备 G-Buffer

```cpp
#include "Renderer/Deferred/FGBufferTextures.h"
#include "Renderer/Deferred/FGBufferFillPass.h"
#include "Renderer/Deferred/FDeferredLightingPass.h"
#include "Renderer/Deferred/FDeferredBlitPass.h"

struct FDeferredShadingTestContext
{
    // ... existing fields ...
    
    // G-Buffer
    FGBufferTextures GBuffer;
    
    // Deferred passes
    TUniquePtr<FGBufferFillPass> GBufferFillPass;
    TUniquePtr<FDeferredLightingPass> LightingPass;
    TUniquePtr<FDeferredBlitPass> BlitPass;
    
    // View constants
    FViewConstants ViewConstants;
    FLightingConstants LightingConstants;
};
```

#### Step 2: 初始化 G-Buffer

```cpp
// After creating device and swapchain

// 1. Initialize G-Buffer textures
if (!Context.GBuffer.Initialize(Context.NvrhiDevice, WIDTH, HEIGHT))
{
    throw runtime_error("Failed to initialize G-Buffer");
}

// 2. Setup view constants
Context.ViewConstants.ViewMatrix = Context.Camera->GetViewMatrix();
Context.ViewConstants.ProjectionMatrix = Context.Camera->GetProjectionMatrix();
Context.ViewConstants.ViewProjectionMatrix = Context.ViewConstants.ViewMatrix * Context.ViewConstants.ProjectionMatrix;
Context.ViewConstants.CameraPosition = Context.Camera->GetPosition();

// 3. Initialize G-Buffer fill pass
Context.GBufferFillPass = MakeUnique<FGBufferFillPass>();
if (!Context.GBufferFillPass->Initialize(
        Context.NvrhiDevice,
        &Context.GBuffer,
        &Context.ViewConstants))
{
    throw runtime_error("Failed to initialize G-Buffer fill pass");
}

// 4. Setup lighting constants
Context.LightingConstants.NumLights = 1;
Context.LightingConstants.Lights[0].Type = LightType_Directional;
Context.LightingConstants.Lights[0].Direction = FVec3(0.5f, -1.0f, 0.3f);
Context.LightingConstants.Lights[0].Color = FVec3(1.0f, 0.9f, 0.8f);
Context.LightingConstants.Lights[0].Intensity = 1.0f;

// 5. Initialize lighting pass
Context.LightingPass = MakeUnique<FDeferredLightingPass>();
if (!Context.LightingPass->Initialize(
        Context.NvrhiDevice,
        &Context.GBuffer,
        nullptr))  // Will create internal output texture
{
    throw runtime_error("Failed to initialize lighting pass");
}
Context.LightingPass->SetLightingConstants(&Context.LightingConstants);

// 6. Initialize blit pass
Context.BlitPass = MakeUnique<FDeferredBlitPass>();
if (!Context.BlitPass->Initialize(
        Context.NvrhiDevice,
        Context.LightingPass->GetOutputTexture()))  // Or use GBuffer output
{
    throw runtime_error("Failed to initialize blit pass");
}
```

#### Step 3: 渲染循环 (Deferred Pipeline)

```cpp
while (TestFrameCount < MaxFrames)
{
    HLVM_ENSURE(Context.DeviceManager->BeginFrame());
    
    nvrhi::IFramebuffer* Framebuffer = Context.DeviceManager->GetCurrentFramebuffer();
    
    // Update camera and matrices
    Context.Camera->UpdateWorldTransform();
    FMat4 Model = glm::rotate(...);
    
    // Update view constants
    Context.ViewConstants.ViewMatrix = Context.Camera->GetViewMatrix();
    Context.ViewConstants.ProjectionMatrix = Context.Camera->GetProjectionMatrix();
    Context.ViewConstants.ModelMatrix = Model;
    Context.ViewConstants.UpdateConstantBuffer(Context.NvrhiCommandList);
    
    Context.NvrhiCommandList->open();
    
    // ===== PHASE 1: GEOMETRY PASS =====
    // Render geometry to G-Buffer
    {
        // Clear G-Buffer
        Context.GBuffer.Clear(Context.NvrhiCommandList);
        
        // Fill G-Buffer with geometry
        Context.GBufferFillPass->Render(Context.NvrhiCommandList);
    }
    
    // ===== PHASE 2: LIGHTING PASS =====
    // Compute lighting from G-Buffer
    {
        Context.LightingPass->Render(Context.NvrhiCommandList);
    }
    
    // ===== PHASE 3: BLIT PASS =====
    // Blit result to screen
    {
        Context.BlitPass->Render(Context.NvrhiCommandList, Framebuffer);
    }
    
    Context.NvrhiCommandList->close();
    Context.NvrhiDevice->executeCommandList(Context.NvrhiCommandList);
    
    Context.DeviceManager->EndFrame();
    Context.DeviceManager->Present();
    
    TestFrameCount++;
}
```

---

### Phase 3: 添加多光源支持 (中期)

**目标**: 支持多个动态光源

```cpp
// Add multiple lights
Context.LightingConstants.NumLights = 4;

// Directional light (sun)
Context.LightingConstants.Lights[0] = {
    .Type = LightType_Directional,
    .Direction = normalize(FVec3(0.5f, -1.0f, 0.3f)),
    .Color = FVec3(1.0f, 0.9f, 0.8f),
    .Intensity = 1.0f
};

// Point light 1
Context.LightingConstants.Lights[1] = {
    .Type = LightType_Point,
    .Position = FVec3(2.0f, 2.0f, 2.0f),
    .Color = FVec3(1.0f, 0.5f, 0.0f),
    .Intensity = 5.0f,
    .Radius = 10.0f
};

// Point light 2
Context.LightingConstants.Lights[2] = {
    .Type = LightType_Point,
    .Position = FVec3(-2.0f, 1.0f, -1.0f),
    .Color = FVec3(0.0f, 0.5f, 1.0f),
    .Intensity = 3.0f,
    .Radius = 8.0f
};

// Spot light
Context.LightingConstants.Lights[3] = {
    .Type = LightType_Spot,
    .Position = FVec3(0.0f, 3.0f, 0.0f),
    .Direction = FVec3(0.0f, -1.0f, 0.0f),
    .Color = FVec3(1.0f, 1.0f, 1.0f),
    .Intensity = 8.0f,
    .InnerConeAngle = glm::radians(15.0f),
    .OuterConeAngle = glm::radians(30.0f)
};
```

---

### Phase 4: 高级特性 (长期)

#### 特性 1: Shadow Mapping
```cpp
// Add shadow map support
struct FShadowMap
{
    nvrhi::TextureHandle DepthTexture;
    nvrhi::FramebufferHandle Framebuffer;
};

// Render depth from light view
RenderShadowMap(Context.NvrhiCommandList, LightViewProj);

// Sample shadows in lighting pass
float shadow = SampleShadowMap(lightDir, depth);
lighting *= (1.0f - shadow * 0.8f);
```

#### 特性 2: Material System
```cpp
// Support multiple materials
struct FMaterial
{
    nvrhi::TextureHandle AlbedoTexture;
    nvrhi::TextureHandle NormalTexture;
    nvrhi::TextureHandle RoughnessTexture;
    nvrhi::TextureHandle MetallicTexture;
    
    float Roughness;
    float Metallic;
};

// Bind material in G-Buffer pass
MaterialBindingSet = CreateMaterialBindingSet(material);
```

#### 特性 3: Post-Processing
```cpp
// Add post-processing effects
ApplyBloom(Context.NvrhiCommandList, lightingOutput);
ApplyToneMapping(Context.NvrhiCommandList, bloomOutput);
ApplySSAO(Context.NvrhiCommandList, gbuffer);
```

---

## 技术细节

### G-Buffer 布局

典型的 G-Buffer 包含:

| 纹理 | 格式 | 内容 |
|------|------|------|
| Albedo | RGBA8_UNORM | Base color + opacity |
| Normal | RGBA8_SNORM | World-space normal |
| Depth | D32_FLOAT | Linear depth |
| Specular | RGBA8_UNORM | Roughness, metallic, AO |
| Velocity | RG16_FLOAT | Motion vectors (for TAA) |

### Deferred Lighting 算法

```glsl
// Simplified deferred lighting compute shader
[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixelCoord = dispatchThreadID.xy;
    
    // Read from G-Buffer
    float4 albedo = AlbedoTexture.Load(pixelCoord);
    float4 normal = NormalTexture.Load(pixelCoord);
    float depth = DepthTexture.Load(pixelCoord).r;
    
    // Reconstruct world position from depth
    float3 worldPos = ReconstructWorldPosition(pixelCoord, depth);
    
    // Calculate lighting
    float3 finalColor = float3(0.0f, 0.0f, 0.0f);
    
    for (int i = 0; i < NumLights; i++)
    {
        Light light = Lights[i];
        
        if (light.Type == LIGHT_DIRECTIONAL)
        {
            float NdotL = saturate(dot(normal.xyz, -light.Direction));
            finalColor += albedo.rgb * light.Color * light.Intensity * NdotL;
        }
        else if (light.Type == LIGHT_POINT)
        {
            float3 lightDir = light.Position - worldPos;
            float distance = length(lightDir);
            
            if (distance < light.Radius)
            {
                lightDir /= distance;
                float NdotL = saturate(dot(normal.xyz, lightDir));
                float attenuation = 1.0f - (distance / light.Radius);
                finalColor += albedo.rgb * light.Color * light.Intensity * NdotL * attenuation;
            }
        }
    }
    
    // Write to output
    OutputTexture[pixelCoord] = float4(finalColor, 1.0f);
}
```

### 性能优化

#### 1. Tile-Based Lighting
```cpp
// Divide screen into tiles (e.g., 16x16 pixels)
// For each tile, determine which lights affect it
// Only process relevant lights per tile

uint tileSize = 16;
uint numTilesX = (width + tileSize - 1) / tileSize;
uint numTilesY = (height + tileSize - 1) / tileSize;

// Build light list per tile
for (uint tileY = 0; tileY < numTilesY; tileY++)
{
    for (uint tileX = 0; tileX < numTilesX; tileX++)
    {
        LightList[tileY][tileX] = FindLightsForTile(tileX, tileY);
    }
}
```

#### 2. Early Z Culling
```cpp
// Enable early Z in pipeline
PipelineDesc.renderState.depthStencilState.setDepthTestEnable(true);
PipelineDesc.renderState.depthStencilState.setEarlyZEnable(true);  // If supported
```

#### 3. MSAA Resolve
```cpp
// If using MSAA, resolve after G-Buffer pass
ResolveMSAA(Context.NvrhiCommandList, gbufferMSAA, gbufferResolved);
```

---

## 实施检查清单

### Phase 1: 修复当前测试

- [ ] 修改 `.sblob` → `.bin` (2 处)
- [ ] 修改 `ConstantBuffer(256)` → `ConstantBuffer(0)` (2 处)
- [ ] 修改 `SetFarPlane(10.0f)` → `SetFarPlane(100.0f)`
- [ ] 重新构建并测试
- [ ] 验证旋转立方体可见

### Phase 2: 实现 Deferred Shading

- [ ] 添加必要的 include
- [ ] 扩展 FDeferredShadingTestContext 结构
- [ ] 初始化 G-Buffer 纹理
- [ ] 初始化 FGBufferFillPass
- [ ] 初始化 FDeferredLightingPass
- [ ] 初始化 FDeferredBlitPass
- [ ] 设置 ViewConstants
- [ ] 设置 LightingConstants (至少 1 个光源)
- [ ] 重写渲染循环为 3-pass 流程
- [ ] 测试并调试

### Phase 3: 增强功能

- [ ] 添加多光源支持
- [ ] 实现材质系统
- [ ] 添加阴影映射
- [ ] 添加后处理效果
- [ ] 性能分析和优化

---

## 常见问题

### Q1: 为什么要用 Deferred Shading?

**A**: 
- ✅ 支持大量光源 (O(lights + geometry) vs O(lights × geometry))
- ✅ 解耦几何和光照
- ✅ 更容易实现复杂的光照模型
- ❌ 更高的内存占用 (G-Buffer)
- ❌ 透明度处理复杂

### Q2: G-Buffer 占用多少内存?

**A**: 对于 1920x1080 分辨率:
```
Albedo:   1920 × 1080 × 4 bytes = 8.3 MB
Normal:   1920 × 1080 × 4 bytes = 8.3 MB
Depth:    1920 × 1080 × 4 bytes = 8.3 MB
Specular: 1920 × 1080 × 4 bytes = 8.3 MB
Total: ~33 MB
```

### Q3: 如何处理透明物体?

**A**: 
1. 先渲染不透明物体到 G-Buffer (deferred)
2. 然后渲染透明物体 (forward)
3. 混合结果

```cpp
// Deferred pass (opaque)
RenderGBuffer(opaqueGeometry);
RenderLighting();

// Forward pass (transparent)
RenderTransparent(forwardGeometry, blending);
```

### Q4: Debug G-Buffer 内容?

**A**: 
```cpp
// Visualize G-Buffer textures
if (debugMode == DEBUG_ALBEDO)
{
    BlitTexture(GBuffer.Albedo, framebuffer);
}
else if (debugMode == DEBUG_NORMAL)
{
    // Convert normals to [0,1] range for visualization
    BlitTextureWithRemap(GBuffer.Normal, framebuffer, [](float3 n) {
        return (n + 1.0f) * 0.5f;
    });
}
```

---

## 参考资源

### HLVM 内部
- [FGBufferFillPass.h](../Engine/Source/Runtime/Public/Renderer/Deferred/FGBufferFillPass.h)
- [FDeferredLightingPass.h](../Engine/Source/Runtime/Public/Renderer/Deferred/FDeferredLightingPass.h)
- [FGBufferTextures.h](../Engine/Source/Runtime/Public/Renderer/Deferred/FGBufferTextures.h)

### Donut 参考
- [Donut Deferred Shading Example](../../../../Gitrepo-Other/Graphics/framework/Donut-Samples/examples/deferred_shading/)
- [GBuffer Implementation](../../../../Gitrepo-Other/Graphics/framework/Donut-Samples/donut/src/engine/GBufferFillPass.cpp)

### 外部资源
- [LearnOpenGL - Deferred Shading](https://learnopengl.com/Advanced-Lighting/Deferred-Shading)
- [NVIDIA Deferred Shading](https://developer.nvidia.com/gpugems/gpugems3/part-ii-light-and-shadows/chapter-10-efficient-transparent-surface-rendering)

---

## 总结

### 当前状态
- ✅ Deferred rendering 基础设施已就绪
- ❌ TestDeferredShading.cpp 只是 forward rendering
- ❌ 存在 binding slot 和文件格式错误

### 下一步
1. **立即**: 修复 Phase 1 的 3 个问题
2. **短期**: 实现 Phase 2 的完整 deferred pipeline
3. **中期**: 添加多光源和材质系统
4. **长期**: 阴影、后处理、优化

### 预期成果
- ✅ 完整的 deferred shading 测试
- ✅ 支持多个动态光源
- ✅ 可扩展的材质系统
- ✅ 良好的性能基准

---

**文档版本**: 1.0  
**创建日期**: 2026-04-21  
**状态**: Ready for Implementation
