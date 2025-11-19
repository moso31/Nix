# MultiFrame（多帧资源）

NXBuffer 是多帧资源：
```C++
	MultiFrame<ComPtr<ID3D12Resource>> m_pBuffer; 
	MultiFrame<ComPtr<ID3D12Resource>> m_pUAVCounterBuffer;
```

## 多帧资源的更新和同步

### 多帧资源更新

目前有两种方法更新多帧资源，如下。

```C++
	void SetCurrent(const void* pSrcData, uint32_t arraySize); // 仅更新当前帧资源
	void SetAll(const void* pSrcData, uint32_t arraySize); // 更新所有的帧资源
```

### 多帧资源同步（底层原理）

同步时走上传系统，将数据 memcpy到 上传ringbuffer的指定区间(src)，然后调用copyBufferRegion()方法，拷贝到本buffer的资源指针。

# 计数器：uavCounter

基本Buffer执行类。主要包括以下资源：
- **m_pBuffer**：一个NXRWBuffer，作为buffer的本体，大小由上层接口设置；
	- 配一个srv+一个uav
- **m_pUAVCounterBuffer**：一个NXRWBuffer，用于统计uavCounter，大小为固定的`sizeof(uint32_t)`
	- 配一个uav

## 底层关键原理

在DX12 API层面，会通过类似这样的代码，在创建UAV时定义二者的“本体/计数器”关系：
```C++
CreateUnorderedAccessView(m_pBuffer, m_pUAVCounterBuffer, ...);
```
这样在GPU更新对应资源时，也会同步更新对应的计数器资源。

### 计数器作用

目前只在处理GPU-Driven Consume/Append Buffer的时候有用。
例如：
- Terrain Fill Ping-pong，当前Buffer的输出是下一个Buffer的输入
	- 需要知道当前Buffer，Pass结束时，被Consume/Append后的资源数量
		- 即uavCounter

这种情况下，在cpu层面，就需要明确这个Buffer的`m_pUAVCounterBuffer`的 UAV。