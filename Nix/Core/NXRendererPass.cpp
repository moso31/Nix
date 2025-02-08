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
	m_psoDesc({}),
	m_passName("Unnamed Pass"),
	m_stencilRef(0x0),
	m_rtSubMeshName("_RenderTarget")
{
	m_psoDesc.InputLayout = NXGlobalInputLayout::layoutPT;
	m_psoDesc.BlendState = NXBlendState<>::Create();
	m_psoDesc.RasterizerState = NXRasterizerState<>::Create();
	m_psoDesc.DepthStencilState = NXDepthStencilState<>::Create();
	m_psoDesc.SampleDesc.Count = 1;
	m_psoDesc.SampleDesc.Quality = 0;
	m_psoDesc.SampleMask = UINT_MAX;
	m_psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	m_srvUavRanges.reserve(1);
}

void NXRendererPass::PushInputTex(NXCommonTexEnum eCommonTex, uint32_t slotIndex)
{
	auto pTex = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(eCommonTex);
	if (m_pInTexs.size() <= slotIndex) m_pInTexs.resize(slotIndex + 1);
	m_pInTexs[slotIndex] = pTex;
}

void NXRendererPass::PushInputTex(const Ntr<NXTexture>& pTex, uint32_t slotIndex)
{
	if (m_pInTexs.size() <= slotIndex) m_pInTexs.resize(slotIndex + 1);
	m_pInTexs[slotIndex] = pTex;
}

void NXRendererPass::PushOutputRT(const Ntr<NXTexture>& pTex)
{
	m_pOutRTs.push_back(pTex);
}

void NXRendererPass::SetOutputDS(const Ntr<NXTexture>& pTex)
{
	m_pOutDS = pTex;
}

void NXRendererPass::SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& desc)
{
	m_psoDesc.InputLayout = desc;
}

void NXRendererPass::SetRenderTargetMesh(const std::string& rtSubMeshName)
{
	m_rtSubMeshName = rtSubMeshName;
}

void NXRendererPass::SetBlendState(const D3D12_BLEND_DESC& desc)
{
	m_psoDesc.BlendState = desc;
}

void NXRendererPass::SetRasterizerState(const D3D12_RASTERIZER_DESC& desc)
{
	m_psoDesc.RasterizerState = desc;
}

void NXRendererPass::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& desc)
{
	m_psoDesc.DepthStencilState = desc;
}

void NXRendererPass::SetSampleDescAndMask(UINT Count, UINT Quality, UINT Mask)
{
	m_psoDesc.SampleDesc.Count = Count;
	m_psoDesc.SampleDesc.Quality = Quality;
	m_psoDesc.SampleMask = Mask;
}

void NXRendererPass::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
{
	m_psoDesc.PrimitiveTopologyType = type;
}

void NXRendererPass::SetShaderFilePath(const std::filesystem::path& shaderFilePath)
{
	m_shaderFilePath = shaderFilePath;
}

void NXRendererPass::InitPSO()
{
	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), m_rootParams, m_staticSamplers);

	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(m_shaderFilePath, "VS", pVSBlob.GetAddressOf());
	NXShaderComplier::GetInstance()->CompilePS(m_shaderFilePath, "PS", pPSBlob.GetAddressOf());

	m_psoDesc.pRootSignature = m_pRootSig.Get();
	m_psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	m_psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };

	m_psoDesc.NumRenderTargets = (UINT)m_pOutRTs.size();
	for (UINT i = 0; i < m_psoDesc.NumRenderTargets; i++)
		m_psoDesc.RTVFormats[i] = m_pOutRTs[i]->GetFormat();
	m_psoDesc.DSVFormat = m_pOutDS.IsNull() ? DXGI_FORMAT_UNKNOWN : m_pOutDS->GetDSVFormat();

	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&m_psoDesc, IID_PPV_ARGS(&m_pPSO));

	std::wstring psoName(NXConvert::s2ws(m_passName) + L" PSO");
	m_pPSO->SetName(psoName.c_str());
}

void NXRendererPass::RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList)
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> ppRTVs;
	for (auto& pTex : m_pOutRTs) ppRTVs.push_back(pTex->GetRTV());
	pCmdList->OMSetRenderTargets((UINT)ppRTVs.size(), ppRTVs.data(), true, m_pOutDS.IsNull() ? nullptr : &m_pOutDS->GetDSV());

	// DX12需要及时更新纹理的资源状态
	for (int i = 0; i < (int)m_pInTexs.size(); i++)
		m_pInTexs[i]->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	for (int i = 0; i < (int)m_pOutRTs.size(); i++)
		m_pOutRTs[i]->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
	if (m_pOutDS.IsValid())
		m_pOutDS->SetResourceState(pCmdList, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void NXRendererPass::RenderBefore(ID3D12GraphicsCommandList* pCmdList)
{
	pCmdList->OMSetStencilRef(m_stencilRef);

	pCmdList->SetGraphicsRootSignature(m_pRootSig.Get());
	pCmdList->SetPipelineState(m_pPSO.Get());

	for (int i = 0; i < (int)m_cbvManagements.size(); i++)
	{
		if (m_cbvManagements[i].autoUpdate)
		{
			const D3D12_GPU_VIRTUAL_ADDRESS gpuVirtAddr = m_cbvManagements[i].multiFrameGpuVirtAddr->Current();
			pCmdList->SetGraphicsRootConstantBufferView(i, gpuVirtAddr);
		}
	}

	if (!m_pInTexs.empty())
	{
		for (int i = 0; i < (int)m_pInTexs.size(); i++) NXShVisDescHeap->PushFluid(m_pInTexs[i]->GetSRV());
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle0 = NXShVisDescHeap->Submit();

		// 2024.6.8
		// 根据目前在.h中的根参数-寄存器布局规定，
		// m_cbvManagements 中 元素的数量就是 Table 的 slot 索引。
		pCmdList->SetGraphicsRootDescriptorTable((UINT)m_cbvManagements.size(), srvHandle0);
	}
}

void NXRendererPass::Render(ID3D12GraphicsCommandList* pCmdList)
{
	NX12Util::BeginEvent(pCmdList, m_passName.c_str());

	RenderSetTargetAndState(pCmdList);
	RenderBefore(pCmdList);

	const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews(m_rtSubMeshName);
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
		// 默认slotIndex = i，可以通过 SetStaticRootParamCBV(, slotIdx, ) 方法修改
		m_rootParams.push_back(NX12Util::CreateRootParameterCBV(i, 0, D3D12_SHADER_VISIBILITY_ALL));
	};

	if (SRVUAVNum)
	{
		m_srvUavRanges.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, SRVUAVNum, 0, 0));

		m_rootParams.push_back(NX12Util::CreateRootParameterTable(m_srvUavRanges, D3D12_SHADER_VISIBILITY_ALL));
	}

	m_cbvManagements.resize(CBVNum);
}

void NXRendererPass::SetStaticRootParamCBV(int rootParamIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs)
{
	m_cbvManagements[rootParamIndex].autoUpdate = true;
	m_cbvManagements[rootParamIndex].multiFrameGpuVirtAddr = gpuVirtAddrs;
}

void NXRendererPass::SetStaticRootParamCBV(int rootParamIndex, int slotIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs)
{
	SetStaticRootParamCBV(rootParamIndex, gpuVirtAddrs);
	m_rootParams[rootParamIndex].Descriptor.ShaderRegister = slotIndex;
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
