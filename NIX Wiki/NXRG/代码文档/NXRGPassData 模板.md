# 概述

用于记录本Pass中依赖的RGHandle。由于不同pass依赖的Handle数量并不一样，因此必须搭载在模板类上才能使用：

```C++
class NXRGPassNodeBase;

template<typename NXRGPassData>
class NXRGPassNode : public NXRGPassNodeBase;
```

# 用法

RenderGraph每次AddPass时，必须在模板传入对应的`<NXRGPassData>`结构体。
该结构体至少需记录当前pass中所有输入、输出的 RGHandle，通常并不需要加入其他内容。具体传什么由开发者自己定义。

该结构体在`setup`和`execute`中均存在，以承担中继作用。
- 在`setup`中，通常作为左值，并负责记录拓扑关系所对应的RGHandle。
- 在`execute`中，通常作为右值，结合`NXRGFrameResource`存储的 **RGHandle-实际资源指针** 拓扑链接，就可以定位到实际渲染器资源指针。

## 代码示例

```C++
struct ShadowTestData // 这就是NXRGPassData
{
	NXRGHandle gbufferDepth;
	NXRGHandle csmDepth;
	NXRGHandle shadowTest;
};

NXRGHandle pShadowTest = m_pRenderGraph->Create("ShadowTest RT", { .resourceType = NXResourceType::Tex2D, .usage = NXRGResourceUsage::RenderTarget, .tex = { .format = DXGI_FORMAT_R11G11B10_FLOAT, .width = (uint32_t)m_viewRTSize.x, .height = (uint32_t)m_viewRTSize.y, .arraySize = 1, .mipLevels = 1 } });

auto shadowTestPassData = m_pRenderGraph->AddPass<ShadowTestData>("ShadowTest",
	[&](NXRGBuilder& builder, ShadowTestData& data) {
		// - 在`setup`中，通常作为左值，并负责记录拓扑关系所对应的RGHandle。
		data.gbufferDepth = builder.Read(gBufferPassData->GetData().depth);
		data.csmDepth = builder.Read(shadowMapPassData->GetData().csmDepth);
		data.shadowTest = builder.Write(pShadowTest);
	},
	[&](ID3D12GraphicsCommandList* pCmdList, const NXRGFrameResources& resMap, ShadowTestData& data) {
		// - 在`execute`中，通常作为右值，结合`NXRGFrameResource`存储的
		// RGHandle-实际资源指针 拓扑链接，就可以定位到实际渲染器资源指针。
		auto pMat = static_cast<NXGraphicPassMaterial*>(m_pPassMaterialMaps["ShadowTest"]);
		pMat->SetConstantBuffer(0, 0, &g_cbObject);
		pMat->SetConstantBuffer(0, 1, &g_cbCamera);
		pMat->SetConstantBuffer(0, 2, &g_cbShadowTest);
		pMat->SetInputTex(0, 0, resMap.GetRes(data.gbufferDepth));
		pMat->SetInputTex(0, 1, resMap.GetRes(data.csmDepth));
		pMat->SetOutputRT(0, resMap.GetRes(data.shadowTest));

		auto vpCamera = NX12Util::ViewPort(m_viewRTSize.x, m_viewRTSize.y);
		pCmdList->RSSetViewports(1, &vpCamera);
		pCmdList->RSSetScissorRects(1, &NX12Util::ScissorRect(vpCamera));
		pMat->Render(pCmdList);
	});
```
