NXRenderGraph的基本运行类。

# 生命周期概述

RenderGraph的特点是每帧动态编译。
由于pass和资源数量本身通常仅在数百/数十个这样的量级，虽然存在不少O(n^2)的运行时逻辑，但并不会带来比较大的性能开销。

每帧运行的时候，会先在Update阶段，清空上一帧的历史记录，然后生成整个RenderGraph，再重新提交编译；然后再进行渲染。
```
void Renderer::Update()
{
	m_pRenderGraph->Clear();
	GenerateRenderGraph(); // 在这里Setup()：构建拓扑关系
	m_pRenderGraph->Compile(); // 在这里Compile()：编译，确定运行时实际需要的D3D资源数量
	...
}

void Renderer::Render()
{
	m_pRenderGraph->Execute(); // 在这里Execute()：调用DX命令队列进行渲染
}
```
# 代码接口

## AddPass(name, setup, execute)

```cpp
template<typename NXRGPassData>
NXRGPassNode<NXRGPassData>* AddPass(
	const std::string& name,
	std::function<void(NXRGBuilder& pBuilder, NXRGPassData& data)> setup,
	std::function<void(ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& frameResources, NXRGPassData& data)> execute)
```
负责在图中传入一个pass。
在`setup`中构建这个图的拓扑关系。
- 注意：AddPass会立刻调用setup方法。
在`execute`中描述如何渲染这一pass。

## Read(resID, passNode)

```cpp
NXRGHandle Read(NXRGHandle resID, NXRGPassNodeBase* passNode);
```
仅在`Setup`中使用。为当前pass构建一条资源输入边。
## Write(passNode, resID)

```cpp
NXRGHandle Write(NXRGPassNodeBase* passNode, NXRGHandle resID);
```
仅在`Setup`中使用。为当前pass构建一条资源输出边。
如果是**Create**类型的资源，调用此接口将会创建**新的Handle版本**。
## ReadWrite(passNode, resID)

```cpp
NXRGHandle ReadWrite(NXRGPassNodeBase* passNode, NXRGHandle resID);
```
仅在`Setup`中使用。为当前pass构建一条资源输出边。
和Write的主要区别在于，对Create类型资源，会强制使用当前的Handle作为输出，不创建新版本。
常用于复写和叠加绘制。
同时该方法也调用一次Read接口，确保编译和GUI显示能得到正确的生命周期。
## Create(name, desc)

```cpp
NXRGHandle Create(const std::string& name, const NXRGDescription& desc);
```
创建一个用于RenderGraph的虚拟资源。
编译RenderGraph时，会根据Read/Write/ReadWrite所建立的连接关系，确定实际需要分配的物理资源。**分配规则见……**
## Import(importResource)

```cpp
NXRGHandle Import(const Ntr<NXResource>& importResource);
```
导入一个用于RenderGraph的资源。只能导入实际存在的资源。
导入的资源是全生命周期存活的，自带物理资源，无需RenderGraph的编译处理。
## Compile()

```cpp
void Compile();
```
在所有拓扑关系完成后调用，主要负责：
- 构建邻接表、入度
- 使用khan拓扑排序计算timeLayer
	- timeLayer是算法假定的虚拟时间点，越接近最终FinalRT的pass，对应的时间值越大。
- 使用贪心算法确认实际分配的资源，如果生命周期不相交就不复用。
## Execute()

```cpp
void Execute();
```
执行每个pass的Execute方法。具体来说需要依赖DX12的命令队列。
目前还没有进行特别完善的封装，很多命令队列的接口直接写在execute中。
## Clear()

```cpp
void Clear();
```
清除各种资源历史状态。

## GetResource(handle)

```cpp
Ntr<NXResource> GetResource(NXRGHandle handle);
```
获取资源对应的渲染器资源指针。

## GetUsingResourceByName(name)

```cpp
Ntr<NXResource> GetUsingResourceByName(const std::string& name);
```
*此接口仅调试使用*
通过名称获取渲染器资源指针。如果有同名的若干资源，随机返回其中的一个。