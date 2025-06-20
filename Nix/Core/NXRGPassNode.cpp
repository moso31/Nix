#include "NXRGPassNode.h"
#include "NXRenderGraph.h"
#include "NXConstantBuffer.h"

NXRGPassNodeBase::NXRGPassNodeBase(NXRenderGraph* pRenderGraph, const std::string& passName, NXRenderPass* pPass) :
	m_pRenderGraph(pRenderGraph), 
	m_passName(passName), 
	m_pPass(pPass), 
	m_indirectArgs(nullptr),
	m_pPassInited(false) 
{
}

void NXRGPassNodeBase::SetRootParamLayout(uint32_t cbvCount, uint32_t srvCount, uint32_t uavCount)
{
	m_rootParamLayout.cbvCount = cbvCount;
	m_rootParamLayout.srvCount = srvCount;
	m_rootParamLayout.uavCount = uavCount;
	m_pPass->SetRootParamLayout(m_rootParamLayout);
}

void NXRGPassNodeBase::Read(NXRGResource* pResource, uint32_t passSlotIndex)
{
	m_inputs.push_back({ pResource, passSlotIndex });
}

void NXRGPassNodeBase::ReadConstantBuffer(uint32_t rootIndex, uint32_t slotIndex, NXConstantBufferImpl* pConstantBuffer)
{
	// 目前的规定是 必须先SetRootParamLayout才能调用ReadConstantBuffer（长期要去掉这个策略吗？）
	assert(m_rootParamLayout.cbvCount > rootIndex);

	m_pPass->SetStaticRootParamCBV(rootIndex, slotIndex, &pConstantBuffer->GetFrameGPUAddresses());
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

NXRGResource* NXRGPassNodeBase::WriteUAV(NXRGResource* pResource, uint32_t uavIndex, bool useOldVersion, uint32_t uavCounterIndex)
{
	// 如果之前没被写入过，那么不需要创建新版本
	if (useOldVersion || !pResource->HasWrited())
	{
		m_outputs.push_back({ pResource, uavIndex, uavCounterIndex });
		pResource->MakeWriteConnect(); // 标记为已写入
		return pResource;
	}

	// 创建新版本
	NXRGResource* pNewVersionResource = new NXRGResource(pResource);
	m_outputs.push_back({ pNewVersionResource, uavIndex, uavCounterIndex });
	pNewVersionResource->MakeWriteConnect(); // 标记为已写入
	m_pRenderGraph->CreateResource(pNewVersionResource->GetName(), pNewVersionResource->GetDescription()); // 添加到graph中
	return pNewVersionResource;
}

NXRGResource* NXRGPassNodeBase::SetIndirectArgs(NXRGResource* pResource)
{
	m_indirectArgs = pResource;
	pResource->MakeWriteConnect(); // 标记为已写入
	return m_indirectArgs;
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
		pPass->SetInputTex(pInResSlot.resource, pInResSlot.slot);
	}

	for (auto pOutResSlot : m_outputs)
	{
		auto pOutRes = pOutResSlot.resource;
		auto flag = pOutRes->GetDescription().handleFlags;
		if (flag == RG_RenderTarget)
		{
			pPass->SetOutputRT(pOutRes, pOutResSlot.slot);
		}
		else if (flag == RG_DepthStencil)
		{
			pPass->SetOutputDS(pOutRes);
		}
	}
}

void NXRGPassNodeBase::Compile_ComputePass(bool isResize)
{
	auto pPass = (NXComputePass*)m_pPass;
	for (auto pInResSlot : m_inputs)
	{
		pPass->SetInput(pInResSlot.resource, pInResSlot.slot);
	}

	for (auto pOutResSlot : m_outputs)
	{
		auto pOutRes = pOutResSlot.resource;
		pPass->SetOutput(pOutRes, pOutResSlot.slot, false);

		if (pOutResSlot.uavCounterSlot != -1)
		{
			// 只有 uav counter 的逻辑才会走这里
			pPass->SetOutput(pOutRes, pOutResSlot.uavCounterSlot, true);
		}
	}

	pPass->SetIndirectArguments(m_indirectArgs);
}
