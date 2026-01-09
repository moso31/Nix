这篇文档描述了从美术资产导出工具，到流式加载，再到GPU-driven计算，最终通过底层gbuffer进行地形实际绘制的工作流。
# 美术资产导出

地形资产导出大致如下，分为四列：
- 第一列：确定美术exr是否存在；
- 第二列：将exr资产（高度图、法线图、splat图）转换成dds。
- 第三列：负责将原始纹理烘焙成子纹理。
- 第四列（除了MinMaxZ均已不再使用）：将dds资产合成全地形2d array
	- 配合旧的全量加载方案使用；
	- 现在仅使用其中的MinMaxZ纹理。

![[image.png|1182x465]]

流式加载主要就是加载第三列：烘焙子区域纹理的若干tile。
烘焙完成后，会在磁盘下保留全地形法线信息。不同地形块的tile存放在不同的编号下。
比如下图就是存放在3_3地形下的部分纹理：

![[image-1.png|571x327]]

# 流式加载系统

流式加载系统负责将上面的各种Tile读取到内存，再经内存转换到显存。

**相关主要c++类：**

- 流式加载主流程大多实现在`NXTerrainLODStreamer`类中
- 数据存放在`NXTerrainLODStreamData`数据类中。
- 异步传输的部分实现在`NXTerrainStreamingAsyncLoader`类中。

## 核心步骤

流式加载的流程大致如下：

**首先获取场景本帧依赖的近处nodes。** 
每级lod的距离不同。比如：
```
static constexpr float DistRanges[6] = { 300.0f, 600.0f, 1200.0f, 2400.0f, 4800.0f, FLT_MAX };
```

**然后为这些nodes构建一个列表出来。**
就正常取出来就行。

**并从粗糙到精细对这些node排序。**
从粗糙（最高lod）到精细（最低lod）逐层构建。
一种常见错觉是为啥不先构建相机附近的，因为没有用——渲染时地形只能从低精度到高精度迭代pingpong，先构建高精度没有意义。

**然后遍历，看node在不在缓存里**
需要在类中提前准备个缓存机制
```
std::vector<NXTerrainLODQuadTreeNodeDescription> m_nodeDescArrayInternal;
```
**然后遍历查前面排序的node，对不在缓存的进行异步构建**
如果node在缓存里，或者正在加载中，就不需要管；
如果不在缓存中，说明没构建过，需要异步构建（然后就是`AsyncLoader`的事情了）

**注1：缓存采用索引固定的LRU Cache**
因为缓存的数据最终实际是要完整传给GPU的，被作为CBuffer传过去。
索引对应了Atlas的映射关系，因此不能sort，否则GPU上65x65x1024的2d array也要跟着变化，就会导致额外开销
- 每个node使用一个time记录最晚更新时间

**注2：如果缓存之前有旧的数据，要记录旧数据的node**。
在异步加载完成时，需要更新旧node对应的Sector2NodeIdTexture的像素。

**注3：异步构建之前，MinMaxZ.dds纹理中，取出当前node对应的最大/最小高度。**
然后通过CBuffer的形式传给GPU，后面patcher阶段用这个判断culling。

**然后由AsyncLoader进行异步构建即可。**
AsyncLoader.cpp不需要mutex，因为`NXManager_Tex->CreateTexture2D`方法本身就是异步的！

**构建时，注意和AsyncLoader同步本次可申请上限**
避免一次push过多任务。
```
int maxRequest = std::max((int)g_terrainStreamConfig.MaxRequestLimit - m_asyncLoader->GetWorkingTaskNum(), 0);
```

**构建完成时的纹理每帧返回到`NXTerrainStreamingAsyncLoader::ConsumeCompletedTasks`的容器中。**
**每帧由渲染器`Renderer.cpp`的NXRG提供三个烘焙pass，获取上述`ConsumeCompletedTasks`容器的纹理并烘焙到对应atlas上。**

之后的流程交给GPU-Driven。

本文和FC5的结构非常相似，流式加载的部分完全交给CPU进行，而gpu-driven culling的部分就交给GPU了
![[image 1.png|601x355]]

# GPU-Driven 及管线流程

**每帧Update时将本帧新的LRU Cache更新到对应GPU CBuffer上**
- 前面也说了缓存的数据是要完整作为CBuffer传到GPU的，具体就是在这里做

**每帧首先需要烘焙，将`AsyncLoader`读取完成的**
即流式加载的最后一步。对应了NXRG的`TerrainAtlasBaker:Height/Splat/Normal`。

**`Terrain Sector2Node Tint`：维护一张Sector2Node索引图，记录世界和node的映射关系。**
每帧都可能烘焙出新纹理，如果一帧是有新纹理的，这个纹理就会更新。
注意：不止画本帧新增加的像素，还会擦旧的像素（如果有）。

**`Terrain Nodes Culling: First`：填充最低精度的nodeID到ping-pong buffer**
把Sector2Node索引图的最后一级mip，对应的每个像素都填到一个Ping pong buf上就行了

**`Terrain Nodes Culling 0~5`：ping pong**
如果子级的四个node全部包含，并且也在对应距离范围内，就ping-pong到下一轮；
否则不再Ping pong，直接输出到final；最后一轮pingpong也会全部强行输出到final。
最后final存的就是本帧可能看到的所有节点。

**Terrain Patcher：可见性剔除** 
基于dtid将每个node拆成8x8的小块；然后对每个块：
- 进行可见性判断
- 作为draw indirect args输出
- （FC5方案还可以做lodTransition补接缝、背面剔除，这里没做）
- 通过测试的patch将会被append到一个patcher buffer里。

**GBuffer：**
使用 patcher buffer 进行渲染即可。

## 关于可见性剔除

目前Nix还不支持多视角，但可以在调试工具中看到实际的剔除效果。
下图就展示了Pix的剔除效果；RenderDoc也可以。
![[image-1 1.png|571x327]]