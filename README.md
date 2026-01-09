# Nix

自制 DX12 渲染器。

## 特性

### GPU Driven 的地形流式加载系统

地形系统，采用流式加载。

整体思路基于 Terrain Rendering in 'Far Cry 5'。详见：https://gdcvault.com/play/1025480/Terrain-Rendering-in-Far-Cry

基于异步的流式加载器传输纹理，并基于GPU-Driven实现剔除优化。

<img width="2560" height="1392" alt="image" src="https://github.com/user-attachments/assets/1a035cfe-5070-45cd-bbde-1abc0ef4766b" />
<img width="2560" height="1392" alt="image" src="https://github.com/user-attachments/assets/c05bec34-ee75-4b9a-96d1-8a6cdccb38bd" />

并提供了一套从Houdini导入资源的完整流程

<img width="1567" height="604" alt="image" src="https://github.com/user-attachments/assets/0218eb3a-d8a5-4ddf-bb4d-5a156b328e45" />

#### 地形系统：性能

在AMD-Ryzen 5800X + RTX 4070Ti环境下，16384x16384的地形，可达720p 1k多帧（cpu瓶颈），2k 550帧，同时显存占用不到200M。并且考虑到代码并非彻底优化，在此基础上仍有不少提升空间。

### NXRG(基于有向无环图的半自动RenderGraph) 

提供了一个半自动的RenderGraph实现。

支持对导入资源自动估算生命周期，并整合同类型纹理实现资源复用，但不支持DAG剪枝优化和断环。

提供了可视化界面，可观察实际资源复用情况

<img width="1986" height="893" alt="image" src="https://github.com/user-attachments/assets/262e5f70-0379-440c-a015-2e78528b1a53" />

<img width="1986" height="893" alt="image" src="https://github.com/user-attachments/assets/c6713628-aa6c-4c06-8fa9-f225da52933b" />

### 标准 PBR

> 延迟渲染管线（Deferred Pipeline）+ 漫反射 IBL（球谐，Spherical Harmonics）+ 预过滤贴图（PreFilter Map）

### Depth Peeling、SSAO

代码DX11时期编写，较老旧，没时间折腾，暂时停用

### 级联阴影贴图（Cascade Shadow Map，仅 PCF）

![image](https://github.com/moso31/Nix/assets/15684115/f3b9d70c-ebb5-4c7e-aa0f-339e62c78115)

### 可编程材质 + HLSL 代码编辑器：[https://github.com/moso31/DonoText](https://github.com/moso31/DonoText)

![image](https://github.com/moso31/Nix/assets/15684115/70bf7f43-61eb-473a-8b7a-5a3fad1230b6)

### Burley SSS + 近似 SSS BTDF

![image](https://github.com/moso31/Nix/assets/15684115/e08205c6-4929-4d25-8019-bcbb7c6a278e)
![transition\_animation](https://github.com/moso31/Nix/assets/15684115/d13bae94-fe1c-4c53-8d0a-be8ed776f075)

### 一个非常简单的色调映射（Tone-Mapping）

### XAllocator

XAllocator 是 Nix DX12 使用的内存分配器，用于为 TextureResources / CBuffers / RTViews / DSViews / Descriptors 分配内存。

Wiki：[https://github.com/moso31/Nix/wiki/XAllocator（还没写完……）](https://github.com/moso31/Nix/wiki/XAllocator（还没写完……）)

## 计划中……

* [ ] 抗锯齿（TAA / SMAA / FXAA……）
* [ ] 屏幕空间环境光遮蔽（HBAO / GTAO……）
* [ ] SSR
* [ ] 更好的后处理（Post-Processing）
