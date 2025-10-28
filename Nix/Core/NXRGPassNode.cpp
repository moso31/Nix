#include "NXRGPassNode.h"
#include "NXRenderGraph.h"
#include "NXConstantBuffer.h"
#include "NXComputePass.h"
#include "NXGraphicPass.h"
#include "NXReadbackData.h"

NXRGPassNodeBase::NXRGPassNodeBase(NXRenderGraph* pRenderGraph, const std::string& passName, NXRGPass* pPass) :
	m_pRenderGraph(pRenderGraph), 
	m_passName(passName), 
	m_pPass(pPass), 
	m_indirectArgs(nullptr),
	m_pPassInited(false) 
{
}

void NXRGPassNodeBase::Read(NXRGResource* pResource, uint32_t passSlotIndex)
{
	m_inputs.push_back({ pResource, passSlotIndex });
}

void NXRGPassNodeBase::ReadConstantBuffer(uint32_t rootIndex, uint32_t slotIndex, uint32_t spaceIndex, NXConstantBufferImpl* pConstantBuffer)
{
	if (!m_pPass->IsRenderPass())
		return;

	auto* pRenderPass = (NXRenderPass*)m_pPass;
	if (pConstantBuffer)
		pRenderPass->SetRootParamCBV(rootIndex, slotIndex, spaceIndex, &pConstantBuffer->GetFrameGPUAddresses());
	else
		pRenderPass->ForceSetRootParamCBV(rootIndex, slotIndex, spaceIndex);
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

NXRGResource* NXRGPassNodeBase::WriteUAV(NXRGResource* pResource, uint32_t uavIndex, bool useOldVersion)
{
	// 如果之前没被写入过，那么不需要创建新版本
	if (useOldVersion || !pResource->HasWrited())
	{
		m_outputs.push_back({ pResource, uavIndex });
		pResource->MakeWriteConnect(); // 标记为已写入
		return pResource;
	}

	// 创建新版本
	NXRGResource* pNewVersionResource = new NXRGResource(pResource);
	m_outputs.push_back({ pNewVersionResource, uavIndex });
	pNewVersionResource->MakeWriteConnect(); // 标记为已写入
	m_pRenderGraph->CreateResource(pNewVersionResource->GetName(), pNewVersionResource->GetDescription()); // 添加到graph中
	return pNewVersionResource;
}

NXRGResource* NXRGPassNodeBase::WriteUAVCounter(NXRGResource* pResource, uint32_t uavCounterIndex)
{
	assert(pResource);
	for (auto& pRes : m_outputs)
	{
		if (pRes.resource == pResource)
		{
			pRes.uavCounterSlot = uavCounterIndex;
			break;
		}
	}
	return pResource;
}

NXRGResource* NXRGPassNodeBase::SetIndirectArgs(NXRGResource* pResource)
{
	m_indirectArgs = pResource;
	pResource->MakeWriteConnect(); // 标记为已写入
	return m_indirectArgs;
}

NXRenderPass* NXRGPassNodeBase::GetRenderPass()
{
	if (!m_pPass->IsRenderPass())
		return nullptr;

	return (NXRenderPass*)m_pPass; 
}

void NXRGPassNodeBase::Compile(bool isResize)
{
	switch (m_pPass->GetPassType())
	{
	case NXRenderPassType::GraphicPass :
		Compile_GraphicsPass(isResize);
		break;
	case NXRenderPassType::ComputePass :
		Compile_ComputePass(isResize);
		break;
	case NXRenderPassType::ReadbackBufferPass :
		Compile_ReadbackBufferPass();
		break;
	}
}

void NXRGPassNodeBase::SetCommandContext(const NXRGCommandContext& ctx)
{
	m_pPass->SetCommandContext(ctx);
}

void NXRGPassNodeBase::Compile_GraphicsPass(bool isResize)
{
	auto pPass = (NXGraphicPass*)m_pPass;
	for (auto pInResSlot : m_inputs)
	{
		pPass->SetInputTex(pInResSlot.resource, pInResSlot.slot, pInResSlot.space);
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
		pPass->SetInput(pInResSlot.resource, pInResSlot.slot, pInResSlot.space);
	}

	for (auto pOutResSlot : m_outputs)
	{
		auto pOutRes = pOutResSlot.resource;
		pPass->SetOutput(pOutRes, pOutResSlot.slot, pOutResSlot.space, false);

		if (pOutResSlot.uavCounterSlot != -1)
		{
			// 只有 uav counter 的逻辑才会走这里
			pPass->SetOutput(pOutRes, pOutResSlot.uavCounterSlot, pOutResSlot.uavCounterSpace, true);
		}
	}

	pPass->SetIndirectArguments(m_indirectArgs);
}

void NXRGPassNodeBase::Compile_ReadbackBufferPass()
{
	// Readback Pass 目前只支持m_input[0] 作为输入，因为本质就是读这个buffer从GPU读到CPU
	if (m_inputs.size() != 1)
		return;

	auto* pPass = (NXReadbackBufferPass*)m_pPass;
	auto* pInRes = m_inputs[0].resource;
	pPass->SetInput(pInRes); // set readback buffer.
	pPass->SetOutput(m_readbackData);
}
