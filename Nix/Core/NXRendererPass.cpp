#include "NXRendererPass.h"
#include "NXResourceManager.h"
#include "ShaderComplier.h"
#include "NXGlobalDefinitions.h"
#include "NXTexture.h"
#include "NXRenderStates.h"
#include "NXSamplerManager.h"
#include "NXAllocatorManager.h"
#include "NXSubMeshGeometryEditor.h"

NXRendererPass::NXRendererPass() :
	m_psoDesc({})
{
	m_srvUavRanges.reserve(1);
}

void NXRendererPass::AddInputTex(NXCommonRTEnum eCommonTex)
{
	m_pInTexs.push_back(NXPassTexture(NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(eCommonTex), eCommonTex));
}

void NXRendererPass::AddOutputRT(NXCommonRTEnum eCommonTex)
{
	m_pOutRTs.push_back(NXPassTexture(NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(eCommonTex), eCommonTex));
}

void NXRendererPass::SetOutputDS(NXCommonRTEnum eCommonTex)
{
	m_pOutDS = NXPassTexture(NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(eCommonTex), eCommonTex);
}

void NXRendererPass::AddInputTex(const Ntr<NXTexture>& pTex)
{
	m_pInTexs.push_back(pTex);
	m_checkStates.inTexNum++;
}

void NXRendererPass::AddOutputRT(const Ntr<NXTexture>& pTex)
{
	m_pOutRTs.push_back(pTex);
	m_checkStates.outRTNum++;
}

void NXRendererPass::SetOutputDS(const Ntr<NXTexture>& pTex)
{
	m_pOutDS = pTex;
	m_checkStates.outDS = m_pOutDS.IsValid();
}

void NXRendererPass::SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& desc)
{
	m_psoDesc.InputLayout = desc;
	m_checkStates.bInputLayout = true;
}

void NXRendererPass::SetBlendState(const D3D12_BLEND_DESC& desc)
{
	m_psoDesc.BlendState = desc;
	m_checkStates.bBlendState = true;
}

void NXRendererPass::SetRasterizerState(const D3D12_RASTERIZER_DESC& desc)
{
	m_psoDesc.RasterizerState = desc;
	m_checkStates.bRasterizerState = true;
}

void NXRendererPass::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& desc)
{
	m_psoDesc.DepthStencilState = desc;
	m_checkStates.bDepthStencilState = true;
}

void NXRendererPass::SetSampleDescAndMask(UINT Count, UINT Quality, UINT Mask)
{
	m_psoDesc.SampleDesc.Count = Count;
	m_psoDesc.SampleDesc.Quality = Quality;
	m_psoDesc.SampleMask = Mask;
	m_checkStates.bSampleDescAndMask = true;
}

void NXRendererPass::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
{
	m_psoDesc.PrimitiveTopologyType = type;
	m_checkStates.bPrimitiveTopologyType = true;
}

void NXRendererPass::SetShaderFilePath(const std::filesystem::path& shaderFilePath)
{
	m_shaderFilePath = shaderFilePath;
	m_checkStates.bShaderFilePath = true;
}

void NXRendererPass::InitPSO()
{
	assert(m_checkStates.Check());

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), m_rootParams, m_staticSamplers);

	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(m_shaderFilePath, "VS", pVSBlob.GetAddressOf());
	NXShaderComplier::GetInstance()->CompilePS(m_shaderFilePath, "PS", pPSBlob.GetAddressOf());

	m_psoDesc.pRootSignature = m_pRootSig.Get();
	m_psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	m_psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };

	m_psoDesc.InputLayout = NXGlobalInputLayout::layoutPT;
	m_psoDesc.BlendState = NXBlendState<>::Create();
	m_psoDesc.RasterizerState = NXRasterizerState<>::Create();
	m_psoDesc.DepthStencilState = NXDepthStencilState<>::Create();
	m_psoDesc.SampleDesc.Count = 1;
	m_psoDesc.SampleDesc.Quality = 0;
	m_psoDesc.SampleMask = UINT_MAX;
	m_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	m_psoDesc.NumRenderTargets = (UINT)m_pOutRTs.size();
	for (UINT i = 0; i < m_psoDesc.NumRenderTargets; i++)
		m_psoDesc.RTVFormats[i] = m_pOutRTs[i]->GetFormat();
	m_psoDesc.DSVFormat = m_pOutDS.IsNull() ? DXGI_FORMAT_UNKNOWN : m_pOutDS->GetFormat();

	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&m_psoDesc, IID_PPV_ARGS(&m_pPSO));
}

void NXRendererPass::RenderBegin(ID3D12GraphicsCommandList* pCmdList)
{
	NX12Util::BeginEvent(pCmdList, m_passName.c_str());

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> ppRTVs;
	for (auto& pTex : m_pOutRTs) ppRTVs.push_back(pTex->GetRTV());

	pCmdList->OMSetRenderTargets((UINT)ppRTVs.size(), ppRTVs.data(), true, m_pOutDS.IsNull() ? nullptr : &m_pOutDS->GetDSV());

	pCmdList->SetGraphicsRootSignature(m_pRootSig.Get());
	pCmdList->SetPipelineState(m_pPSO.Get());

	for (int i = 0; i < m_checkStates.cbvNum; i++)
		pCmdList->SetGraphicsRootConstantBufferView(i, m_cbvGpuVirtAddrs[i]);

	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle0;
	if (!m_pInTexs.empty())
	{
		srvHandle0 = NXGPUHandleHeap->SetFluidDescriptor(m_pInTexs[0]->GetSRV());
		for (int i = 1; i < (int)m_pInTexs.size(); i++) NXGPUHandleHeap->SetFluidDescriptor(m_pInTexs[i]->GetSRV());

		pCmdList->SetGraphicsRootDescriptorTable(m_checkStates.cbvNum, srvHandle0);
	}
}

void NXRendererPass::RenderEnd(ID3D12GraphicsCommandList* pCmdList)
{
	const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews("_RenderTarget");
	pCmdList->IASetVertexBuffers(0, 1, &meshView.vbv);
	pCmdList->IASetIndexBuffer(&meshView.ibv);
	pCmdList->DrawIndexedInstanced(meshView.indexCount, 1, 0, 0, 0);

	NX12Util::EndEvent(pCmdList);
}

void NXRendererPass::SetRootParams(int CBVNum, int SRVUAVNum)
{
	m_srvUavRanges.clear();
	m_rootParams.clear();
	for (int i = 0; i < CBVNum; i++)
	{
		m_rootParams.push_back(NX12Util::CreateRootParameterCBV(i, 0, D3D12_SHADER_VISIBILITY_ALL));
	};

	if (SRVUAVNum)
	{
		m_srvUavRanges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SRVUAVNum, 0, 0));

		m_rootParams.push_back(NX12Util::CreateRootParameterTable(m_srvUavRanges, D3D12_SHADER_VISIBILITY_ALL));
	}

	m_cbvGpuVirtAddrs.resize(CBVNum);
	m_checkStates.cbvNum = CBVNum;
	m_checkStates.srvUavNum = SRVUAVNum;
}

void NXRendererPass::SetRootParamCBV(int rootParamIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuVirtAddr)
{
	assert(rootParamIndex >= 0 && rootParamIndex < m_checkStates.cbvNum);
	m_cbvGpuVirtAddrs[rootParamIndex] = gpuVirtAddr;
}

void NXRendererPass::AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& samplerDesc)
{
	m_staticSamplers.push_back(samplerDesc);
}

void NXRendererPass::AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW)
{
	auto& samplerDesc = NXSamplerManager::GetInstance()->CreateIso((int)m_staticSamplers.size(), 0, D3D12_SHADER_VISIBILITY_ALL, filter, addrUVW);
	m_staticSamplers.push_back(samplerDesc);
}

void NXRendererPass::OnResize()
{
	for (auto& pTex : m_pInTexs)
	{
		if (pTex.IsCommonRT())
			pTex.pTexture = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(pTex.rtType);
	}

	for (auto& pTex : m_pOutRTs)
	{
		if (pTex.IsCommonRT())
			pTex.pTexture = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(pTex.rtType);
	}

	if (m_pOutDS.IsCommonRT())
		m_pOutDS.pTexture = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(m_pOutDS.rtType);
}
