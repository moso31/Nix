#include "NXComputePass.h"
#include "NXResourceManager.h"

NXComputePass::NXComputePass() :
	m_csoDesc({})
{
	m_csoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
}

void NXComputePass::SetInputTex(NXCommonTexEnum eCommonTex, uint32_t slotIndex)
{
	auto pTex = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(eCommonTex);
	if (m_pInRes.size() <= slotIndex) m_pInRes.resize(slotIndex + 1);
	m_pInRes[slotIndex] = pTex;
}

void NXComputePass::SetInputTex(const Ntr<NXTexture>& pTex, uint32_t slotIndex)
{
	if (m_pInRes.size() <= slotIndex) m_pInRes.resize(slotIndex + 1);
	m_pInRes[slotIndex] = pTex;
}

void NXComputePass::SetOutputTex(const Ntr<NXTexture>& pTex, uint32_t slotIndex)
{
	if (m_pOutRes.size() <= slotIndex) m_pOutRes.resize(slotIndex + 1);
	m_pOutRes[slotIndex] = pTex;
}

void NXComputePass::SetInputBuffer(const Ntr<NXBuffer>& pBuffer, uint32_t slotIndex)
{
	if (m_pInRes.size() <= slotIndex) m_pInRes.resize(slotIndex + 1);
	m_pInRes[slotIndex] = pBuffer;
}

void NXComputePass::SetOutputBuffer(const Ntr<NXBuffer>& pBuffer, uint32_t slotIndex)
{
	if (m_pOutRes.size() <= slotIndex) m_pOutRes.resize(slotIndex + 1);
	m_pOutRes[slotIndex] = pBuffer;
}

void NXComputePass::SetRootParams(int CBVNum, int SRVNum, int UAVNum)
{
	m_sr.clear();
	m_rootParams.clear();
	for (int i = 0; i < CBVNum; i++)
	{
		// 默认slotIndex = i，可以通过 SetStaticRootParamCBV(, slotIdx, ) 方法修改
		m_rootParams.push_back(NX12Util::CreateRootParameterCBV(i, 0, D3D12_SHADER_VISIBILITY_ALL));
	};

	if (SRVNum)
	{
		m_srvUavRanges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SRVUAVNum, 0, 0));

		m_rootParams.push_back(NX12Util::CreateRootParameterTable(m_srvUavRanges, D3D12_SHADER_VISIBILITY_ALL));
	}

	m_cbvManagements.resize(CBVNum);
}
