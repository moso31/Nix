#include "NXRGPassNode.h"
#include "NXRenderGraph.h"

void NXRGPassNodeBase::Read(NXRGResource* pResource, uint32_t passSlotIndex)
{
	m_inputs.push_back({ pResource, passSlotIndex });
}

NXRGResource* NXRGPassNodeBase::WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool useOldVersion)
{
	// 如果之前没被写入过，那么不需要创建新版本
	if (useOldVersion || !pResource->HasWrited())
	{
		m_outputs.push_back({ pResource, outRTIndex });
		pResource->MakeWriteConnect(); // 标记为已写入
		return pResource;
	}

	// 创建新版本
	NXRGResource* pNewVersionResource = new NXRGResource(pResource);
	m_outputs.push_back({ pNewVersionResource, outRTIndex });
	pNewVersionResource->MakeWriteConnect(); // 标记为已写入
	m_pRenderGraph->CreateResource(pNewVersionResource->GetName(), pNewVersionResource->GetDescription()); // 添加到graph中
	return pNewVersionResource;
}

NXRGResource* NXRGPassNodeBase::WriteDS(NXRGResource* pResource, bool useOldVersion)
{
	// 如果要求保留像素，或之前没被写入过，那么不需要创建新版本
	if (useOldVersion || !pResource->HasWrited())
	{
		m_outputs.push_back({ pResource, uint32_t(-1) });
		pResource->MakeWriteConnect(); // 标记为已写入
		return pResource;
	}

	// 创建新版本
	NXRGResource* pNewVersionResource = new NXRGResource(pResource);
	m_outputs.push_back({ pNewVersionResource, uint32_t(-1) });
	pNewVersionResource->MakeWriteConnect(); // 标记为已写入
	m_pRenderGraph->CreateResource(pNewVersionResource->GetName(), pNewVersionResource->GetDescription()); // 添加到graph中
	return pNewVersionResource;
}

NXRGResource* NXRGPassNodeBase::WriteUAV(NXRGResource* pResource, uint32_t outUAVIndex, bool useOldVersion)
{
	// 如果之前没被写入过，那么不需要创建新版本
	if (useOldVersion || !pResource->HasWrited())
	{
		m_outputs.push_back({ pResource, outUAVIndex });
		pResource->MakeWriteConnect(); // 标记为已写入
		return pResource;
	}

	// 创建新版本
	NXRGResource* pNewVersionResource = new NXRGResource(pResource);
	m_outputs.push_back({ pNewVersionResource, outUAVIndex });
	pNewVersionResource->MakeWriteConnect(); // 标记为已写入
	m_pRenderGraph->CreateResource(pNewVersionResource->GetName(), pNewVersionResource->GetDescription()); // 添加到graph中
	return pNewVersionResource;
}

void NXRGPassNodeBase::Compile(bool isResize)
{
	m_pPass->GetPassType() == NXRenderPassType::GraphicPass ? Compile_GraphicsPass(isResize) : Compile_ComputePass(isResize);
}

void NXRGPassNodeBase::Compile_GraphicsPass(bool isResize)
{
	auto pPass = (NXGraphicPass*)m_pPass;
	for (auto pInResSlot : m_inputs)
	{
		pPass->SetInputTex(pInResSlot.resource->GetResource(), pInResSlot.slot);
	}

	for (auto pOutResSlot : m_outputs)
	{
		auto pOutRes = pOutResSlot.resource;
		auto flag = pOutRes->GetDescription().handleFlags;
		if (flag == RG_RenderTarget)
		{
			pPass->SetOutputRT(pOutRes->GetResource(), pOutResSlot.slot);
		}
		else if (flag == RG_DepthStencil)
		{
			pPass->SetOutputDS(pOutRes->GetResource());
		}
	}

	if (!isResize)
		pPass->SetupInternal();
}

void NXRGPassNodeBase::Compile_ComputePass(bool isResize)
{
	auto pPass = (NXComputePass*)m_pPass;
	for (auto pInResSlot : m_inputs)
	{
		pPass->SetInput(pInResSlot.resource->GetResource(), pInResSlot.slot);
	}

	for (auto pOutResSlot : m_outputs)
	{
		auto pOutRes = pOutResSlot.resource;
		pPass->SetOutput(pOutRes->GetResource(), pOutResSlot.slot);
	}

	if (!isResize)
		pPass->SetupInternal();
}
