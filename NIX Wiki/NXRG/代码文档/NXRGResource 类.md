NXRGResource的虚拟资源类。

# 概述

NXRGResource代表RenderGraph中的虚拟资源，每个资源有自己的名称、描述和版本号。

虚拟资源是RenderGraph在Setup阶段构建的资源抽象，并不直接对应实际的GPU资源。在Compile阶段，RenderGraph会根据资源的生命周期和描述信息，将多个虚拟资源映射到实际的物理资源（`Ntr<NXResource>`），实现资源复用优化。

# 代码接口

## 构造函数（创建新版本）

```cpp
NXRGResource(NXRGResource* pOldResource)
```

基于已有资源创建新版本。主要用于`Write()`操作，当对Create类型的资源执行写入时调用。

**特点**：
- 继承旧资源的名称和描述
- 保持相同的索引（index），版本号+1
- 更新全局最大版本号记录（`s_maxVersions`）

## 构造函数（创建新资源）

```cpp
NXRGResource(const std::string& name, const NXRGDescription& description, bool isImported)
```

创建全新的虚拟资源。用于`Create()`和`Import()`操作。

**参数**：
- `name`：资源名称，用于调试和GUI显示
- `description`：资源描述信息（格式、尺寸等），Create资源需要此信息分配物理资源
- `isImported`：是否为外部导入资源。Import资源直接使用已有的物理资源，不参与资源复用优化

**特点**：
- 分配新的全局唯一索引（`s_nextIndex++`）
- 初始版本号为0
- 记录到全局最大版本号映射表

## GetName()

```cpp
const std::string& GetName();
```

获取资源名称。主要用于调试输出和GUI显示。

## GetDescription()

```cpp
const NXRGDescription& GetDescription();
```

获取资源描述信息。Compile阶段用此信息创建或查找可复用的物理资源。

**注**：Import资源创建时需要给一个空的Description，但不会使用（Import资源已有物理资源实例）。

## GetHandle()

```cpp
NXRGHandle GetHandle();
```

获取资源句柄。Handle是RenderGraph中资源的唯一标识符，包含索引和版本号。

**用途**：
- Setup阶段：通过Handle构建Pass之间的资源依赖关系
- Compile阶段：通过Handle追踪资源生命周期，决定物理资源分配策略
- Execute阶段：通过Handle查询实际的物理资源指针（`NXRGFrameResources::GetRes(handle)`）

## IsImported()

```cpp
bool IsImported();
```

判断是否为导入资源。

**用途**：
- Write操作时：Import资源不创建新版本，直接返回当前Handle
- Compile阶段：Import资源跳过生命周期分析和物理资源分配，直接使用已有资源
- 资源管理：Import资源的生命周期由外部控制，RenderGraph不负责创建和销毁