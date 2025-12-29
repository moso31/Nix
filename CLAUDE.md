# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

Nix 是一个自制的 DirectX 12 渲染器，使用 C++ 编写，具有先进的基于物理的渲染（PBR）功能。项目使用 Visual Studio 2019/2022 和 Windows 10 SDK，通过 MSBuild 构建。

**主要功能：**
- 标准 PBR 延迟管线，配备漫反射 IBL（球谐函数）和预过滤贴图
- 带 PCF 的级联阴影贴图
- 深度剥离实现顺序无关透明
- Burley SSS（次表面散射）和近似 SSS BTDF
- 带 HLSL 代码编辑器的可编程材质系统
- 渲染图系统（NXRG）用于自动资源管理
- GPU 驱动的 LOD 地形流式渲染
- 自定义内存分配器（XAllocator）用于 DX12 资源

## 构建和开发命令

### 构建项目
```bash
# 构建主项目（Debug, x64）
msbuild Nix.sln /p:Configuration=Debug /p:Platform=x64

# 构建 Release 版本
msbuild Nix.sln /p:Configuration=Release /p:Platform=x64

# 构建 FullyDebug 配置（用于深度调试）
msbuild Nix.sln /p:Configuration=FullyDebug /p:Platform=x64

# 重新构建（清理 + 构建）
msbuild Nix.sln /p:Configuration=Debug /p:Platform=x64 /t:Rebuild
```

### 运行测试
```bash
# 构建并运行 FunctionTest 项目
msbuild Nix.sln /p:Configuration=Debug /p:Platform=x64 /t:FunctionTest
cd x64/Debug
FunctionTest.exe
```

`FunctionTest` 项目包含内存分配器的单元测试（BuddyAllocTest、DeadListAllocTest）。

### 解决方案项目结构

解决方案包含以下项目：
- **Nix**（主可执行文件）：渲染器应用程序
- **NixMath**：数学库（向量、矩阵等）
- **DirectXTex**：微软的纹理加载/处理库
- **IMGUI**：Dear ImGui GUI 库
- **XAllocator**：自定义 DX12 内存分配器系统
- **NXDebugLogOutput**：调试日志工具
- **FunctionTest**：分配器和工具的单元测试

## 架构概览

### 核心类层次结构

项目遵循基于继承的架构，以 `NXObject` 为中心：

```
IRefCountable → NXRefCountable → NXObject
                                   ├── NXResource（纹理、缓冲区）
                                   ├── NXMaterial（PBR 材质）
                                   ├── NXScene（场景管理）
                                   └── NXTransform（空间层次结构）
                                       ├── NXCamera
                                       ├── NXCubeMap
                                       └── NXRenderableObject
                                           ├── NXPrimitive
                                           ├── NXPrefab
                                           └── NXTerrain
```

关键基类：
- `NXObject`：带引用计数的基础资源类（使用 `Ntr<T>` 智能指针）
- `NXSerializable`：序列化/反序列化接口
- `NXResource`：GPU 资源基类（继承自 NXObject 和 NXSerializable）
- `NXRefCountable`：引用计数实现

### 智能指针系统：Ntr<T>

代码库使用自定义智能指针 `Ntr<T>`（类似 `std::shared_ptr`）用于引用计数对象。对所有继承自 `NXRefCountable` 的对象使用此指针：

```cpp
Ntr<NXTexture2D> texture = ...;
Ntr<NXPBRMaterial> material = ...;
```

### 渲染图系统（NXRG）

渲染管线使用**渲染图**模式进行自动资源管理和 Pass 调度：

**核心组件：**
- `NXRenderGraph`：管理 Pass 和资源的主图
- `NXRGPassNode<T>`：类型化渲染 Pass 节点，带 setup/execute 回调
- `NXRGBuilder`：在 Pass setup 中使用，声明资源依赖
- `NXRGHandle`：虚拟资源的不透明句柄
- `NXRGResource`：带生命周期跟踪的虚拟资源包装器

**资源管理：**
- `Create()`：创建新的虚拟资源（可以被别名/重用）
- `Import()`：导入外部资源（不由 RG 管理）
- `Read()`：声明 Pass 读取资源
- `Write()`：声明 Pass 写入资源（创建新版本）
- `ReadWrite()`：声明 Pass 读写资源

渲染图自动完成：
- 基于依赖关系对 Pass 进行拓扑排序
- 计算资源生命周期
- 使用贪心算法对不重叠资源进行别名/重用内存
- 生成 GPU 资源转换

**渲染图工作流程：**

位于 `Nix/Core/Renderer.cpp` 的 `GenerateRenderGraph()` 构建完整的帧图：
1. 导入外部资源（cubemaps、BRDF LUT 等）
2. 通过 `AddPass<PassData>(name, setup, execute)` 添加 Pass
3. 在 setup lambda 中：通过 builder 声明资源读/写
4. 在 execute lambda 中：使用命令列表执行实际渲染
5. `Compile()` 构建依赖图并分配资源
6. `Execute()` 按拓扑顺序运行所有 Pass

### XAllocator 内存系统

用于高效 GPU 资源管理的自定义 DX12 内存分配器：

**关键分配器：**
- `PlacedBufferAllocator`：放置缓冲区的 Buddy 分配器
- `CommittedBufferAllocator`：用于提交的缓冲区资源
- `DescriptorAllocator`：管理描述符堆分配（SRV/UAV/CBV/RTV/DSV）
- `DeadList`：延迟删除资源（等待 GPU 完成）
- `Buddy`：二进制 Buddy 分配器实现

**分配器架构：**
- 位于 `XAllocator/` 目录
- 提供基于池的分配以减少 D3D API 调用
- 处理资源删除的 GPU/CPU 同步
- 由主项目中的 `NXAllocatorManager` 管理

### 资源管理器模式

每种资源类型都有专用的单例管理器：
- `NXTextureResourceManager`：纹理加载和缓存
- `NXMaterialResourceManager`：材质实例和模板
- `NXMeshResourceManager`：网格/几何数据
- `NXLightResourceManager`：光源管理
- `NXCameraResourceManager`：相机实例

所有管理器都继承自 `NXResourceManagerBase`，通过 `NXResourceManager` 访问。

### 着色器系统

**着色器文件：**
- 位于 `Nix/Shader/` 目录
- `.fx` 扩展名的 HLSL 着色器
- 通用着色器工具在 `Common.fx`、`Math.fx`、`BRDF.fx` 中

**着色器编译：**
- 由 `ShaderComplier` 类处理
- 使用 D3DCompileFromFile 在运行时编译
- 支持带可编程着色器编辑器的自定义材质着色器

**关键着色器 Pass：**
- `GBufferEasy.fx`：延迟渲染的 G-buffer 生成
- `DeferredRender.fx`：延迟光照 Pass
- `ShadowMap.fx`：阴影贴图生成
- `ShadowTest.fx`：阴影测试/采样
- `ForwardTranslucent.fx`：透明物体的前向渲染
- `GPUTerrainPatcher.fx`：GPU 驱动的地形 LOD 剔除
- `VTReadback.fx`：虚拟纹理反馈回读

### GUI 系统

项目使用 **Dear ImGui** 作为编辑器 GUI：

**主 GUI 类：** `NXGUI`（Nix/Core/NXGUI.h/cpp）
- 管理所有 GUI 面板和窗口
- 与 DirectX 12 渲染器集成

**GUI 模块**（位于 `Nix/Core/NXGUI*.h`）：
- `NXGUIInspector`：对象属性检查器
- `NXGUIMaterial`：材质编辑器
- `NXGUIMaterialShaderEditor`：自定义材质的 HLSL 代码编辑器
- `NXGUIRenderGraph`：渲染图可视化和调试
- `NXGUITerrainSystem`：地形 LOD 流式控制
- `NXGUIContentExplorer`：资源浏览器
- `NXGUICodeEditor`：语法高亮代码编辑器

### GPU 驱动的地形系统

受 Far Cry 5 启发的高级 LOD 地形渲染：

**核心概念：**
- 基于四叉树的 LOD，使用 ping-pong consume/append 缓冲区
- 通过计算着色器进行 GPU 剔除
- 流式系统根据相机距离加载/卸载地形瓦片
- 基于 NodeID 的间接寻址用于虚拟纹理

**关键类：**
- `NXTerrain`：CPU 端地形实体
- `NXGPUTerrainManager`：GPU 端地形块管理
- `NXTerrainLODStreamer`：异步地形数据流式传输
- `NXTerrainStreamingAsyncLoader`：后台线程加载地形数据
- `NXTerrainLODStreamData`：单个地形瓦片数据

**实现细节：**
- 使用 160x160 的 R16_UNORM 纹理，每个纹素存储一个 NodeID
- NodeDescriptionArray 将 NodeID 映射到瓦片数据（minmaxZ、LOD bias、atlas ID）
- Terrain Fill Pass 执行 ping-pong 迭代以确定可见的 LOD 块
- GPU Terrain Patcher 将块细分为 8x8 个 patch 并执行视锥体剔除
- 详细文档见 `NIX Wiki/NX Gpu-Driven Terrain.md`

### 常量缓冲区

使用模板化的 `NXConstantBuffer<T>` 包装器（Nix/Nix/NXConstantBuffer.h）：

```cpp
struct CBufferData { ... };
NXConstantBuffer<CBufferData> cbuffer;
cbuffer.SetData(data);
cbuffer.Upload();  // 上传到 GPU
```

常量缓冲区结构应使用 `struct_cbuffer` 宏进行正确对齐：

```cpp
struct_cbuffer CBufferData {
    Matrix view;
    Vector4 color;
};
```

## 代码位置模式

**核心引擎代码：** `Nix/Core/`
- 入口点：`Main.cpp`
- 主渲染器：`Renderer.h/cpp`
- 应用程序：`App.h/cpp`

**场景和对象：** `Nix/Nix/`
- 场景管理：`NXScene.h/cpp`
- 相机：`NXCamera.h/cpp`
- 材质：`NXPBRMaterial.h/cpp`、`NXPassMaterial.h/cpp`
- 几何体：`NXSubMesh.h/cpp`、`NXPrimitive.h/cpp`

**资源系统：** `Nix/Nix/`
- `NXTexture.h/cpp`：纹理基类
- `NXBuffer.h/cpp`：缓冲区基类
- `NXResource.h/cpp`：资源基类

**渲染子系统：** `Nix/Core/`
- 渲染图：`NXRenderGraph.h/cpp`、`NXRGPassNode.h/cpp`
- 阴影映射：`Renderer.cpp` 中的 CSM 实现
- 深度预渲染：`NXDepthPrepass.h/cpp`
- SSAO：`NXSimpleSSAO.h/cpp`

**实用工具和辅助：** `Nix/Core/`
- 输入处理：`NXInput.h/cpp`
- 计时器：`NXTimer.h/cpp`
- 随机数：`NXRandom.h/cpp`
- 文件 I/O：`NXFileSystemHelper.h/cpp`

**基础定义：** `Nix/BaseDefs/`
- 核心包含：`NixCore.h`
- DirectX 12：`DX12.h/cpp`
- 数学类型：`Math.h`（使用 DirectXMath SimpleMath）

## 重要编码模式

### 资源状态转换

DX12 需要显式的资源状态转换。使用：

```cpp
resource->SetResourceState(pCommandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
```

`NXResource` 基类跟踪当前状态并自动生成屏障。

### DirectX 12 对象

对 D3D12 COM 对象使用 Microsoft::WRL::ComPtr：

```cpp
Microsoft::WRL::ComPtr<ID3D12Resource> resource;
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList;
```

### 内存管理宏

遗留清理宏（代码库中仍在使用）：

```cpp
SafeDelete(ptr);         // delete + nullptr
SafeDeleteArray(arr);    // delete[] + nullptr
SafeRelease(obj);        // obj->Release() + delete + nullptr
```

对于托管对象，优先使用 `Ntr<T>`。

### 调试配置

解决方案有三种构建配置：
- **Debug**：标准调试构建，禁用优化
- **FullyDebug**：重度调试（额外验证，更慢）
- **Release**：用于性能测试的优化构建

## 外部依赖

**通过解决方案管理：**
- DirectXTex（在 `DirectXTex/` 子目录中）
- Dear ImGui（在 `IMGUI/` 项目中）
- DirectXMath（SimpleMath 包装器）

**所需 SDK：**
- Windows 10 SDK
- DirectX 12 Agility SDK（通过 D3D12.dll）
- FBX SDK（用于 `FBXMeshLoader`）

## 开发说明

### 添加新的渲染 Pass

1. 在 `Renderer.cpp` 中定义 Pass 数据结构
2. 通过 `m_pRenderGraph->AddPass<PassData>(name, setup, execute)` 添加 Pass
3. 在 setup lambda 中：使用 builder 声明资源（`builder.Read/Write/Create`）
4. 在 execute lambda 中：设置管线、绑定资源、绘制
5. 渲染图自动处理资源生命周期和转换

### 创建自定义材质

材质使用 `NXPassMaterial` 系统：
- 每个材质定义顶点/像素着色器路径
- 着色器通过 `ShaderComplier` 在运行时编译
- 通过常量缓冲区暴露材质参数
- 参见 `NXGUIMaterialShaderEditor` 了解可编程着色器编辑

### 处理地形

地形系统复杂。关键点：
- 使用 `NXTerrainLODStreamer` 进行异步瓦片加载
- GPU Pass 在 `GPUTerrainPatcher.fx` 计算着色器中
- 地形配置在 `NXTerrainLODStreamConfigs.h` 中
- NodeID 纹理和描述数组由 `NXGPUTerrainManager` 管理

### 调试可视化

通过渲染器启用调试层：
- `SetEnableDebugLayer(true)`：叠加调试信息
- `SetEnableShadowMapDebugLayer(true)`：可视化级联阴影贴图
- 渲染图可视化在 `NXGUIRenderGraph` 中可用

## 常见开发工作流程

### 运行应用程序
1. 在 Visual Studio 2019+ 中打开 `Nix.sln`
2. 将配置设置为 Debug/x64
3. 构建解决方案（F7）或"生成"→"生成解决方案"
4. 运行（F5）或"调试"→"开始调试"
5. 应用程序创建一个带 ImGui 编辑器界面的窗口

### 调试 GPU 问题
- 使用 PIX for Windows 进行图形调试
- 在调试构建中启用 D3D12 调试层（在 `DX12.cpp` 中设置）
- 检查 GUI 中的渲染图可视化以查看资源依赖关系
- 使用 `NXGUIRenderGraph` 检查 Pass 执行顺序和资源生命周期

### 着色器修改
- 编辑 `Nix/Shader/` 目录中的 `.fx` 文件
- 着色器在运行时编译 - 重启应用程序以查看更改
- 使用材质着色器编辑器 GUI 进行实时着色器编辑
- 在调试输出窗口中检查着色器编译错误
