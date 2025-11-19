# 主要流程

**生成邻接表和入度**
- 遍历每个pass
	- 先遍历输入，再遍历输出
	- 遍历输入时
		- 构建一条**最后写入该资源的pass->当前pass**的邻接边
		- 同时当前pass入度+1；
	- 遍历输出时
		- 用一个lastWriteMap明确最后写入该资源的pass=当前pass

**拓扑排序（kahn sort）**
- 基于各pass的邻接表和入度进行排序
	- 设置一个queue
		- 遍历每个pass，将入度=0的pass推入queue
		- 无限while，直到queue不空
			- 从queue队头移出一个pass（如果需要记录topo顺序，就再用一个vector，按序存这pass）
			- 遍历当前pass的邻接表v，从中找一个最大的timelayer，如果都没有当前pass的layer+1大，就用当前pass
				- 公式：layer[v]=max(layer[pass], layer[v]+1) 
				- v的入度-1，如果v入度=0，说明已经没有前继节点了，推到queue里
				- 返回上面的无限while
		
结束时得到一个拓扑排序好的图，每个pass记录自己的timeLayer

**确认layer**
然后使用一个O(n^2)的算法确定各resource的起止layer

**生成实际资源，并构建NXRGHandle和实际资源的映射表**
基于贪心算法，看哪些Handle对应的timelayer不重叠但desc兼容。
只要不重叠+[[#怎样才算兼容|兼容]]，那么就可以使用同一个资源。
这样就能确认具体生成的资源数量。

## 细节解释

每个RenderGraph链接，都是`start-resource-pass-resource-pass-resource-end`的链接关系。

编译时，将`resource`的概念剔除了，主要确认pass之间的连接关系

那么怎么抽象出pass间的连接呢？
**假设纹理用字母表示，pass用汉字表示**
如：`a-甲-b-乙-c, a-甲-d-丙-c`
那么我们只需关注`甲-乙`、`甲-丙`连接，把资源`abcd`这些抽象掉。
将pass连接关系存储成邻接表+入度即可。

然后对pass连接进行拓扑排序。
其实我们想要的，只是给每个pass上一个time值，确保所有pass，都有 先执行的time<后执行的time 就可以。
kahn排序完美契合这一需求。
具体的，设置一个队列，然后持续运行，每当图中有入度为0的节点，就加到队列中。
然后不断的FIFO取出队头元素pass，并找这个pass的邻接pass
- 然后更新邻接pass的time值，每次更新必须比传入节点的time和自己原本的time都高 
- 然后让邻接pass入度-1。
- 持续迭代直到整个图迭代完成（所有节点入度=0），就获得了一个time值。

>*如果是一个pass仅有输出资源，没有输入资源，那么该资源的time start=end，在时间轴上为一个点。*
### 怎样才算兼容

如果两个资源都不兼容，那还重叠个der。

每个纹理资源都会存一个描述，根据这个描述算哈希值。
描述中描述了这个资源的细节信息。

注意下算兼容这一块，基本只处理Create类型的资源。
- 对Create类型的资源
	- 类型：Texture还是Buffer
	- Texture
		- 类型：1D、2D、2DArray、Cube、3D
		- 宽高深
		- format
		- 用途：RT、DS、常规
	- Buffer
		- stride
		- bytesize

Import纹理的生命周期交给外部维护。
如果出现builder.Write(Import)这样的情况，会创建新的Handle，但**不会为这部分Handle分配新的内存**。

# 对导入资源的处理

导入资源是输入资源。
