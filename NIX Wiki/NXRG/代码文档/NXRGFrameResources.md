# 作用

每个Pass都持有一个`NXRGFrameResources`实例，负责记录**虚拟资源句柄（NXRGHandle）**和**物理资源指针（Ntr\<NXResource\>）**的映射关系。

## 三个阶段的作用

**Setup阶段**：通过`NXRenderGraph::Read()/Write()/ReadWrite()`构建Pass之间的资源依赖关系（拓扑连接）。

**Compile阶段**：RenderGraph根据拓扑连接确定每个Pass的输入输出Handle，然后将这些Handle与Compile分配的物理资源建立映射，通过`Register()`注册到各个Pass的`NXRGFrameResources`中。

**Execute阶段**：Pass的执行函数通过`GetRes(handle)`查询虚拟句柄对应的实际物理资源，进行渲染操作。

这种设计使得Setup阶段可以使用抽象的Handle构建逻辑，而Execute阶段能访问到正确版本的物理资源，避免了资源版本错配的问题。

# 代码接口

## Register(handle, pResource)

```cpp
void Register(NXRGHandle handle, const Ntr<NXResource>& pResource);
```

将虚拟资源句柄与物理资源指针建立映射关系。

**调用时机**：仅在`Compile()`阶段由RenderGraph内部调用。

**作用**：每个Pass持有独立的`NXRGFrameResources`实例，映射关系是Per-Pass的。同一个物理资源可能在不同Pass中对应不同的Handle版本。

## GetRes(handle)

```cpp
Ntr<NXResource> GetRes(NXRGHandle handle) const;
```

通过虚拟句柄获取对应的物理资源指针。

**调用时机**：仅在`Execute()`阶段由Pass的执行函数调用。

**注意**：必须使用Setup阶段通过`Read()/Write()/ReadWrite()`返回的Handle来查询，否则会触发断言错误。这保证了每个Pass访问的是正确版本的资源。

**示例**：
```cpp
// Execute函数中
Ntr<NXTexture> pRT = resMap.GetRes(data.rt0);
pRT->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
```

## Clear()

```cpp
void Clear();
```

清除当前Pass的所有Handle-Resource映射。

**调用时机**：在`Compile()`开始前由RenderGraph调用（通过`ClearBeforeCompile()`），清理上一帧的映射记录，为新一帧的编译做准备。