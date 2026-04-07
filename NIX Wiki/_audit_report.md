# Wiki 审查报告
> 生成时间：2026-04-07

## 摘要
- 已审查文档：14 篇
- 需要更新（已完成）：3 篇
- 已过时：0 篇
- 状态良好：9 篇
- 新发现：4 篇（材质系统文档）
- 待审查：1 篇（没解决的问题）

## 详细发现

### 已更新的文档

#### [[NXBuffer]]
- **问题**：缺少异步视图创建（promise/future 模式）和 Fence 同步机制的记录；SetResourceState() 对 UAVCounter 的描述不准确
- **修复**：补充了异步视图创建、Fence 同步两个章节；修正了 SetResourceState 行为描述
- **涉及代码**：`NXBuffer.h/.cpp`

#### [[NXReadback]]
- **问题**：内容过于简略（约10%覆盖率），缺少 ring buffer 机制、线程模型、API 说明、已知限制
- **修复**：扩充了 NXReadbackSystem（ring buffer 64MB、16 任务槽、fence 轮询）、NXReadbackPassMaterial（Buffer/Texture 两条路径、API）、NXReadbackData（线程安全说明）
- **涉及代码**：`NXReadbackSystem.h/.cpp`, `NXPassMaterial.h`, `NXReadbackData.h`

#### [[Adaptive Virtual Texture]]
- **问题**：缺少物理页面 padding 尺寸、每帧烘焙限制、Sector2VirtImg 编码格式
- **修复**：补充了 264x264 padding、每帧 4 页面 / 64 像素限制、32-bit 编码格式
- **涉及代码**：`NXVirtualTexture.h`, `NXVirtualTextureCommon.h`

### 状态良好

#### [[NX 继承链]]
- 所有 28 条继承关系均与代码一致，未发现遗漏的新类

#### [[NXRG/NXRG Compile 拓扑排序]]
- Kahn 排序算法描述、资源生命周期 O(n^2)算法、贪心别名策略均与代码一致

#### [[NXRG/代码文档/文档汇总]]
- 7 篇子文档的方法签名、类描述均与代码一致
- 注：GUI 调试接口（GetGUIVirtualResources 等）未文档化，属内部实现，不影响准确性

#### [[NX 地形系统/NX 地形系统-工作流]]
- 核心架构准确（距离范围、粗到细排序、LRU 缓存、ping-pong 机制）
- 小补充：clarified MaxRequestLimit/MaxComputeLimit 各为 16

#### [[NX 地形系统/NX 地形系统-开销]]
- 所有纹理尺寸、格式、大小计算均与代码一致

### 新发现文档

发现 `NX 材质系统/` 下存在 4 篇文档（含目录），之前未被索引系统收录：
- [[NX 材质系统/NX 材质系统-架构]] — 类层次与着色模型
- [[NX 材质系统/NX 材质系统-NSL 与编译]] — NSL 格式与编译管线
- [[NX 材质系统/NX 材质系统-数据流]] — 材质生命周期与 GUI 热编辑
- 这 3 篇文档原为 GB2312 编码，已修复为 UTF-8

### 待审查

#### [[没解决的问题]]
- 属于问题追踪文档，需要人工判断哪些问题已解决

## 覆盖度变化

- 从"未覆盖子系统"中移除：PBR 材质系统（已有文档）、着色器编译（NSL 文档覆盖）
- 新增未覆盖子系统：GPU 性能分析（`NXGPUProfiler.h`）

## 编码修复

以下 9 个文件从 GB2312 转换为 UTF-8（无 BOM）：
- `_index.md`, `_changelog.md`
- `wiki-audit.prompt.md`, `wiki-update.prompt.md`, `wiki-sync.prompt.md`, `wiki-new.prompt.md`
- `NX 材质系统-NSL 与编译.md`, `NX 材质系统-数据流.md`, `NX 材质系统-架构.md`