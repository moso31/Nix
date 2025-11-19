# 概述

NXRGPassNode代表RenderGraph中的一个渲染Pass，存储Pass的输入输出资源以及执行逻辑。
必须搭载模板类才能使用。详见 [[NXRGPassData 模板]] 
```C++
class NXRGPassNodeBase;

template<typename NXRGPassData>
class NXRGPassNode : public NXRGPassNodeBase;
```

# 代码接口

## NXRGPassNodeBase

### 构造函数

```cpp
NXRGPassNodeBase(NXRenderGraph* pRenderGraph, const std::string& passName)
```

### GetName()

```cpp
const std::string& GetName();
```

### Execute(pCmdList)

```cpp
virtual void Execute(ID3D12GraphicsCommandList* pCmdList) = 0;
```

### AddInput(resHandle)

```cpp
void AddInput(NXRGHandle resHandle);
```

### AddOutput(resHandle)

```cpp
void AddOutput(NXRGHandle resHandle);
```

### GetInputs()

```cpp
const std::vector<NXRGHandle>& GetInputs();
```

### GetOutputs()

```cpp
const std::vector<NXRGHandle>& GetOutputs();
```

### RegisterFrameResource(handle, pRes)

```cpp
void RegisterFrameResource(NXRGHandle handle, const Ntr<NXResource>& pRes);
```
注册**帧资源。**
### ClearBeforeCompile()

```cpp
void ClearBeforeCompile();
```

## NXRGPassNode (模板类)

### 构造函数

```cpp
NXRGPassNode(NXRenderGraph* pRenderGraph, const std::string& passName)
```

### GetData()

```cpp
NXRGPassData& GetData();
```

### Execute(pCmdList)

```cpp
void Execute(ID3D12GraphicsCommandList* pCmdList) override;
```

### RegisterExecuteFunc(func)

```cpp
void RegisterExecuteFunc(std::function<void(ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& frameResourcesMap, NXRGPassData& data)> func);
```

