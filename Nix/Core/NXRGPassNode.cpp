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
	// Ŀǰ�Ĺ涨�� ������SetRootParamLayout���ܵ���ReadConstantBuffer������Ҫȥ����������𣿣�
	assert(m_rootParamLayout.cbvCount > rootIndex);

	m_pPass->SetStaticRootParamCBV(rootIndex, slotIndex, &pConstantBuffer->GetFrameGPUAddresses());
}

NXRGResource* NXRGPassNodeBase::WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool useOldVersion)
{
	// ���֮ǰû��д�������ô����Ҫ�����°汾
	if (useOldVersion || !pResource->HasWrited())
	{
		m_outputs.push_back({ pResource, outRTIndex });
		pResource->MakeWriteConnect(); // ���Ϊ��д��
		return pResource;
	}

	// �����°汾
	NXRGResource* pNewVersionResource = new NXRGResource(pResource);
	m_outputs.push_back({ pNewVersionResource, outRTIndex });
	pNewVersionResource->MakeWriteConnect(); // ���Ϊ��д��
	m_pRenderGraph->CreateResource(pNewVersionResource->GetName(), pNewVersionResource->GetDescription()); // ��ӵ�graph��
	return pNewVersionResource;
}

NXRGResource* NXRGPassNodeBase::WriteDS(NXRGResource* pResource, bool useOldVersion)
{
	// ���Ҫ�������أ���֮ǰû��д�������ô����Ҫ�����°汾
	if (useOldVersion || !pResource->HasWrited())
	{
		m_outputs.push_back({ pResource, uint32_t(-1) });
		pResource->MakeWriteConnect(); // ���Ϊ��д��
		return pResource;
	}

	// �����°汾
	NXRGResource* pNewVersionResource = new NXRGResource(pResource);
	m_outputs.push_back({ pNewVersionResource, uint32_t(-1) });
	pNewVersionResource->MakeWriteConnect(); // ���Ϊ��д��
	m_pRenderGraph->CreateResource(pNewVersionResource->GetName(), pNewVersionResource->GetDescription()); // ��ӵ�graph��
	return pNewVersionResource;
}

NXRGResource* NXRGPassNodeBase::WriteUAV(NXRGResource* pResource, uint32_t uavIndex, bool useOldVersion, uint32_t uavCounterIndex)
{
	// ���֮ǰû��д�������ô����Ҫ�����°汾
	if (useOldVersion || !pResource->HasWrited())
	{
		m_outputs.push_back({ pResource, uavIndex, uavCounterIndex });
		pResource->MakeWriteConnect(); // ���Ϊ��д��
		return pResource;
	}

	// �����°汾
	NXRGResource* pNewVersionResource = new NXRGResource(pResource);
	m_outputs.push_back({ pNewVersionResource, uavIndex, uavCounterIndex });
	pNewVersionResource->MakeWriteConnect(); // ���Ϊ��д��
	m_pRenderGraph->CreateResource(pNewVersionResource->GetName(), pNewVersionResource->GetDescription()); // ��ӵ�graph��
	return pNewVersionResource;
}

NXRGResource* NXRGPassNodeBase::SetIndirectArgs(NXRGResource* pResource)
{
	m_indirectArgs = pResource;
	pResource->MakeWriteConnect(); // ���Ϊ��д��
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
			// ֻ�� uav counter ���߼��Ż�������
			pPass->SetOutput(pOutRes, pOutResSlot.uavCounterSlot, true);
		}
	}

	pPass->SetIndirectArguments(m_indirectArgs);
}
