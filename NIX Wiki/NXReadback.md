NXReadback负责从CPU读取Readback相关的内容。
## NXReadbackSystem
围绕ringbuffer的传输系统。
## NXReadbackBufferPass
NXRGPass节点的派生类，详见[[NXRG]]。
- 内置一个GPUBuffer
- 每帧走传输系统，动态创建task读取这个GPUBuffer的值
	- 传输系统将GPUBuffer转化成这个[[#^0|CPUData]] ^1
- 依赖FinishTask内置的lambda对CPUData进行更新。
## NXReadbackData
基本回读数据类型。
- 每帧由NXReadbackBufferPass，将GPU结果读出到这个[[#^1|CPUData]] ^0
- 在更上层，至少GUI能拿到的地方，保留一个该data的指针
	- 然后GUI解析这个data，就可以将数据可视化