# 描述

NXRenderGraph中的基本虚拟资源句柄。

包含一个**索引（index）**和一个**版本号（version）**：
- **索引**：标识资源的唯一ID。每个`Create()`创建的资源会获得全局唯一索引（`s_nextIndex++`）。相同索引的不同版本表示同一逻辑资源在管线不同Pass中的演化状态。
- **版本号**：标识该`Create`资源的不同版本。初始版本号为0。后续每次调用`Write()`时会创建新版本（索引相同，版本号+1）。这允许RenderGraph追踪资源在多个Pass间的写入历史，用于资源生命周期管理和物理资源复用优化。

注意：
- 每帧都会重新创建所有的NXRGHandle。
	- 但如果管线结构不变化，获得的NXRGHandle通常都一致。
- `Import()`创建的资源会获得一个全局唯一索引，但不会使用版本号。

**示例**：
- 资源A首次通过Create创建：Handle(index=5, version=0)
- PassB对A执行Write：生成新Handle(index=5, version=1)
- PassC对A再次Write：生成新Handle(index=5, version=2)
