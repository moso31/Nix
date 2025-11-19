# 核心思路

- Terrain Fill ping-pong pass
	- 使用 Consume/Append Buffer + ping-pong迭代
		- 每轮ping-pong收缩评估距离，并将距离外的block计入`int3[]`
		- 首轮block的面积为整块地形（2048），后续每轮宽高减半
	- 最终的`int3[]`：**z = 对应逆序lod等级，xy = 对应lod等级的块坐标**
- GPU Terrain Patcher
	- `int3[]`中的每个元素进一步划分成8x8个patch
	- 每个patch和Frustum求交，剔除
	- 记录每个patch的：
		- 变换矩阵
		- sliceIndex【导致了过高的显存占用】

【后续：梳理完整个流程，然后结合gpt分析下，怎么和所谓的四叉树节点+流式的概念结合】

按照FC5原文的说法，我对它的理解如下：
- 将每个tile全量加载到显存中的话，会有数千个（显存开销非常高），所以需要一个LOD系统。
- 我们将地形数据存在一个四叉树结构中。根节点的数据常驻；根据LOD和玩家的距离流式传输这些tile。
- 这样通常仅需驻留500个tile左右（不同项目情况不同）。

同时，GPU层面准备两个数据结构
- 一个160x160，带mip，R16_UNORM的纹理（需根据项目大世界实际大小调整纹理大小）
	- 每个纹素R16记录一个nodeID
- 一个NodeDescriptionArray
	- 前文的nodeID就是这里Array的索引
	- 每个元素存储minmaxZ/lodbias/AtlasID等数据
		- **cpu做一个NodeDescriptionArray的影子复制，方便跟踪分配情况**

CPU只负责流式加载/卸载对应纹理，按照自己的规则设置