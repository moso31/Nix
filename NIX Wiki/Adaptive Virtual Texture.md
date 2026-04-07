# 原理

定义**sector=世界空间下64x64大小的区域**。
将渲染对象切割的非常细碎，切成很多个小page。每个小page都是pageTable里的一个像素单元。
pageTable在概念上被视作四叉树结构，但实际数据结构是一张带Mip的纹理。
- pageTable四叉树的特性是，当分配一个Node时，表示这个Node=一个sector。
	- 每个Node包含很多个page
	- 物理页面实际大小为 256x256 + 4px 边界 padding = 264x264
离相机越近的sector可以分配到越大的Node
- Sector2VirtImg：一个映射关系，记录这个Sector使用的四叉树Node（通过角位置、大小表示）
- Migrate(Upscale/DownScale)
	- 升采样/降采样

### 回读
从GBuffer定期回读pageID数据
- 1/8 RT+BayerDither
- 每个像素记录所用pageID+mip+size
- 去重

### 烘焙
- 结合四叉树，从pageID、mip、size可以反推Sector
	- sector、pageID、mip、size 共同构成一个 key
- 每帧限制：最多烘焙 **4 个物理页面**，更新 **64 个间接纹理像素**
- Sector2VirtImg 编码格式：32-bit（X\[11:0\] | Y\[23:12\] | log2Size\[31:24\]）
- 使用LRU维护key
	- 如果一个key在LRU是已有的
		- 不一定跳过
		- 还要看sector版本是否变化（由于流式加载异步特性引入的特性）
			- 解决方案：持续跟踪每个sector和LRU的每个key的版本号
			- 如果key数据不变，但版本号变了，也需要重新bake key
		- 维护页表，更新1像素【但LRU索引并没有变，因此可能没有用，但写一下也没有什么坏处】
	- 如果key不在LRU中b
		- LRU维护：去掉老key，放新的key
			- 维护页表，删除1像素
			- bake key
			- 维护页表，更新1像素

### GBuffer
- 读：PhysicalPage、pageTable、Sector2VirtImg
- 逻辑链：
	- 当前像素-posWS
		- posWS % 64 = sectorUV
	- m_VTSector2VirtImg：从posWS->四叉树/页表的角位置
		- 结合sectorUV：定位具体的pageID.xy
		- 结合pageTable\[pageID.xy\]=slotIndex
		- PhysicalPage\[slotIndex\]=目标纹理

### 状态机

状态机是很推荐的策略。
如果不做一个同步状态的话，会出现很多问题：
- **四叉树**是当前帧的，但**回读pageId**因为存在异步特性，依赖的可能是前好几帧的四叉树。烘焙page时会同时依赖**四叉树**和**回读page**，但二者数据不同步，导致结果出错
- 如果一帧灌入过多bake page请求，会导致单帧负载过高

# 结果展示

