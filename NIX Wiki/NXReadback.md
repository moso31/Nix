NXReadback 负责从 GPU 读取回读数据到 CPU。

## NXReadbackSystem

围绕 ring buffer 的传输系统。
- Ring buffer 大小 **64MB**，分配对齐 **512 字节**
- 固定 **16 个并发任务槽**（`TASK_NUM = 16`），超出时阻塞等待
- 两阶段任务生命周期：
	1. `BuildTask()`：分配 ring buffer 空间，返回 `NXReadbackContext`
	2. `FinishTask()`：注册完成回调，绑定 GPU fence 值
- 线程同步使用 mutex + condition_variable
- `UpdatePendingTaskFenceValue()` 在帧末调用，记录当前 GPU fence
- `Update()` 轮询 `GetCompletedValue()` 触发回调

## NXReadbackPassMaterial

NXPassMaterial 的派生类（位于 NXPassMaterial.h），用于 Readback 相关的 Pass 材质。
- 配合 NXReadbackSystem 使用
- 每帧走传输系统，动态创建 task 读取 GPUBuffer 的值
	- 传输系统将 GPUBuffer 转化成这个 [[#^0|CPUData]] ^1
- 依赖 FinishTask 内置的 lambda 对 CPUData 进行更新。

### 两条回读路径
- **Buffer 路径**（`ReadbackBuffer()`）：`CopyBufferRegion()` 到 ring buffer
- **Texture 路径**（`ReadbackTexture()`）：使用 `GetCopyableFootprints()` 计算布局，逐行拷贝
	- 当前仅支持 **2D 纹理的 mip0**

### API
```C++
SetInput(Ntr<NXResource> pRes)              // GPU 资源
SetOutput(Ntr<NXReadbackData>& pOutData)    // CPU 数据容器
SetCallback(std::function<void()>)          // 自定义完成回调
```

## NXReadbackData

基本回读数据类型。
- 每帧由 Readback 系统，将 GPU 结果读出到这个 [[#^1|CPUData]] ^0
- 在更上层，至少 GUI 能拿到的地方，保留一个该 data 的指针
	- 然后 GUI 解析这个 data，就可以将数据可视化

### 线程安全
- `CopyDataFromGPU()`：仅在 `Update()` 回调中执行（回读线程）
- `Get()`：**非线程安全**，仅用于预览/快照接口
- `Clone()`：**线程安全**，mutex 保护的深拷贝，多线程访问时应使用此方法