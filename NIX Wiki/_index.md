# NIX Wiki 索引

> 本文件是 NIX Wiki 的总索引，记录每篇文档覆盖的代码范围和最后同步状态。
> 由 wiki 维护系统自动更新，也可手动编辑。

## 索引表

| Wiki 文档 | 覆盖范围 | 关联代码文件 | 最后同步 | 状态 |
|-----------|---------|-------------|---------|------|
| [[NX 继承链]] | 类继承关系图 | `Nix/Nix/NXObject.h`, `NXRefCountable.h`, `NXResource.h`, `NXTransform.h`, `NXMaterial`, `NXPassMaterial.h` 等 | 2026-04-07 | current |
| [[NXBuffer]] | 多帧缓冲区管理、Fence 同步、异步视图 | `Nix/Nix/NXBuffer.h/.cpp` | 2026-04-07 | current |
| [[NXReadback]] | GPU->CPU 回读系统 | `Nix/Core/NXReadbackSystem.h`, `Nix/Nix/NXReadbackData.h`, `Nix/Nix/NXPassMaterial.h` | 2026-04-07 | current |
| [[Adaptive Virtual Texture]] | 虚拟纹理系统 | `Nix/Nix/NXVirtualTexture*`, `Nix/Shader/VT*.fx` | 2026-04-07 | current |
| [[NX 地形系统/目录]] | 地形系统总览 | - | 2026-04-07 | current |
| [[NX 地形系统/NX 地形系统-工作流]] | 地形流式/GPU 驱动管线 | `Nix/Nix/NXTerrain*`, `Nix/Shader/Terrain*.fx` | 2026-04-07 | current |
| [[NX 地形系统/NX 地形系统-开销]] | 地形 VRAM/性能预算 | `Nix/Nix/NXTerrainLODStreamConfigs.h` | 2026-04-07 | current |
| [[NXRG/NXRG Compile 拓扑排序]] | 渲染图编译与拓扑排序 | `Nix/Core/NXRenderGraph.h/.cpp` | 2026-04-07 | current |
| [[NXRG/代码文档/文档汇总]] | NXRG 代码级文档 | `Nix/Core/NXRenderGraph.h`, `NXRGPassNode.h`, `NXRGBuilder.h`, `NXRGResource.h` | 2026-04-07 | current |
| [[NX 材质系统/目录]] | 材质系统总览 | - | 2026-04-07 | new |
| [[NX 材质系统/NX 材质系统-架构]] | 类层次、数据类型、着色模型 | `NXPBRMaterial.h`, `NXPassMaterial.h`, `NXCodeProcessHeader.h` | 2026-04-07 | new |
| [[NX 材质系统/NX 材质系统-NSL 与编译]] | NSL 格式、解析流程、Root Signature | `NXCodeProcessHelper.h/.cpp`, `NXShaderComplier` | 2026-04-07 | new |
| [[NX 材质系统/NX 材质系统-数据流]] | 加载->绑定->渲染工作流、GUI 编辑器 | `NXMaterialResourceManager.h`, `NXGUIMaterial.h`, `NXGUIMaterialShaderEditor.h` | 2026-04-07 | new |
| [[没解决的问题]] | 已知问题追踪 | 多个子系统 | 2026-04-07 | needs-review |

## 未覆盖的代码子系统

以下子系统目前没有对应的 wiki 文档：

| 子系统 | 关键代码 | 优先级 |
|--------|---------|--------|
| 延迟渲染管线 | `Nix/Shader/GBufferEasy.fx`, `DeferredRender.fx`, `Renderer.cpp` | 高 |
| 阴影系统 (CSM) | `Nix/Shader/ShadowMap.fx`, `ShadowTest.fx` | 中 |
| GUI 系统 | `Nix/Core/NXGUI*.h` (25+ 面板) | 中 |
| 资源管理器 | `NXResourceManager.h`, `NXTextureResourceManager.h` 等 | 中 |
| GPU 性能分析 | `NXGPUProfiler.h`, `NXGUIGPUProfiler.h` | 中 |
| 智能指针 Ntr<T> | `Nix/Core/Ntr.h` | 低 |
| XAllocator 内存系统 | `XAllocator/` | 低 |
| 后处理 (SSAO/SSS) | `NXSimpleSSAO.h`, `NXDepthPrepass.h`, `SSSSSRenderer.fx` | 低 |
| 深度剥离 (OIT) | `NXDepthPeelingRenderer.h`, `DepthPeelingCombine.fx` | 低 |
| 场景与序列化 | `NXScene.h`, `NXSerializable.h` | 低 |
| Upload 系统 | `NXUploadSystem.h` | 低 |
| PSO 管理 | `NXPSOManager.h` | 低 |

## 状态说明

- **current** — wiki 与代码一致
- **needs-review** — 代码可能已变化，需要审查
- **outdated** — 已确认过时，需要更新
- **new** — 新发现/新创建，待验证