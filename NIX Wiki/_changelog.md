# NIX Wiki 变更日志

> 记录 wiki 的每次更新。由 wiki 维护系统自动追加。

## [2026-04-07] sync | 全量同步 + 编码修复
- 修复 9 个文件的编码问题（GB2312 -> UTF-8 无 BOM）
- 更新 [[NXBuffer]]：补充异步视图创建、Fence 同步章节；修正 SetResourceState 描述
- 更新 [[NXReadback]]：大幅扩充，补充 ring buffer 机制、线程模型、API、两条回读路径、线程安全说明
- 更新 [[Adaptive Virtual Texture]]：补充物理页面 padding、每帧烘焙限制、Sector2VirtImg 编码格式
- 更新 [[NX 地形系统/NX 地形系统-工作流]]：补充 MaxRequestLimit/MaxComputeLimit 参数
- 发现并收录 [[NX 材质系统]] 4 篇文档到索引
- 更新 _index.md：新增材质系统条目，移除已覆盖子系统，新增 GPU 性能分析
- 生成 _audit_report.md 审查报告

## [2026-04-07] init | Wiki 维护系统初始化
- 创建 _index.md 总索引，登记 10 篇现有文档
- 创建 _changelog.md 变更日志
- 识别 13 个未覆盖的代码子系统