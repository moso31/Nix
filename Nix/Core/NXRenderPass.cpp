#include "NXRenderPass.h"
#include "NXSamplerManager.h"
#include "NXGlobalDefinitions.h"

NXRenderPass::NXRenderPass(NXRenderPassType type) :
	NXRGPass(type),
	m_entryNameVS(L"VS"),
	m_entryNamePS(L"PS"),
	m_entryNameCS(L"CS")
{
}

void NXRenderPass::SetRootParamCBV(int rootParamIndex, int slotIndex, int spaceIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs)
{
	if (m_cbvManagements.size() <= rootParamIndex) m_cbvManagements.resize(rootParamIndex + 1);
	if (m_rootParams.size() <= rootParamIndex) m_rootParams.resize(rootParamIndex + 1);

	m_cbvManagements[rootParamIndex].autoUpdate = true;
	m_cbvManagements[rootParamIndex].multiFrameGpuVirtAddr = gpuVirtAddrs;
	m_cbvManagements[rootParamIndex].cbvSlot = slotIndex;
	m_cbvManagements[rootParamIndex].cbvSpace = spaceIndex;

	m_rootParamLayout.cbvCount = std::max(m_rootParamLayout.cbvCount, rootParamIndex + 1);
}

void NXRenderPass::ForceSetRootParamCBV(int rootParamIndex, int slotIndex, int spaceIndex)
{
	if (m_cbvManagements.size() <= rootParamIndex) m_cbvManagements.resize(rootParamIndex + 1);
	if (m_rootParams.size() <= rootParamIndex) m_rootParams.resize(rootParamIndex + 1);
	m_cbvManagements[rootParamIndex].cbvSlot = slotIndex;
	m_cbvManagements[rootParamIndex].cbvSpace = spaceIndex;

	m_rootParamLayout.cbvCount = std::max(m_rootParamLayout.cbvCount, rootParamIndex + 1);
}

void NXRenderPass::AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& samplerDesc)
{
	m_staticSamplers.push_back(samplerDesc);
}

void NXRenderPass::AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW)
{
	auto& samplerDesc = NXSamplerManager::GetInstance()->CreateIso((int)m_staticSamplers.size(), 0, D3D12_SHADER_VISIBILITY_ALL, filter, addrUVW);
	m_staticSamplers.push_back(samplerDesc);
}

void NXRenderPass::InitRootParams()
{
	m_rootParams.resize(m_rootParamLayout.cbvCount + (int)m_rootParamLayout.srvCount.size() + (int)m_rootParamLayout.uavCount.size());
	
	// 清空并重新分配 descriptor ranges，确保生命周期正确
	m_srvRanges.resize(m_rootParamLayout.srvCount.size());
	m_uavRanges.resize(m_rootParamLayout.uavCount.size());
	
	int rootParamIndex = 0;

	for (int i = 0; i < m_rootParamLayout.cbvCount; i++)
	{
		m_rootParams[rootParamIndex++] = NX12Util::CreateRootParameterCBV(m_cbvManagements[i].cbvSlot, m_cbvManagements[i].cbvSpace, D3D12_SHADER_VISIBILITY_ALL);
	};

	for (int space = 0; space < m_rootParamLayout.srvCount.size(); space++)
	{
		int slotNum = m_rootParamLayout.srvCount[space];
		m_srvRanges[space] = NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, slotNum, 0, space);
		m_rootParams[rootParamIndex++] = NX12Util::CreateRootParameterTable(1, &m_srvRanges.back(), D3D12_SHADER_VISIBILITY_ALL);
	}

	for (int space = 0; space < m_rootParamLayout.uavCount.size(); space++)
	{
		int slotNum = m_rootParamLayout.uavCount[space];
		m_uavRanges[space] = NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, slotNum, 0, space);
		m_rootParams[rootParamIndex++] = NX12Util::CreateRootParameterTable(1, &m_uavRanges.back(), D3D12_SHADER_VISIBILITY_ALL);
	}

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), m_rootParams, m_staticSamplers);
}
