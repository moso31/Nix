#include "NXSkyRenderer.h"
#include "ShaderComplier.h"
#include "NXGlobalDefinitions.h"
#include "NXRenderStates.h"
#include "NXSamplerManager.h"
#include "DirectResources.h"
#include "NXResourceManager.h"
#include "NXTexture.h"
#include "NXAllocatorManager.h"

#include "NXScene.h"
#include "NXCubeMap.h"

NXSkyRenderer::NXSkyRenderer(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXSkyRenderer::~NXSkyRenderer()
{
}

void NXSkyRenderer::InitSignature()
{
	std::vector<D3D12_DESCRIPTOR_RANGE> ranges = {
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0)
	};

	std::vector<D3D12_ROOT_PARAMETER> rootParam = {
		NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterCBV(1, 0, D3D12_SHADER_VISIBILITY_ALL),
		NX12Util::CreateRootParameterTable(ranges, D3D12_SHADER_VISIBILITY_ALL)
	};

	std::vector<D3D12_STATIC_SAMPLER_DESC> samplers = {
		NXSamplerManager::GetInstance()->CreateIso(0, 0, D3D12_SHADER_VISIBILITY_ALL)
	};

	NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), rootParam, samplers);
}

void NXSkyRenderer::InitPSO()
{
	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\CubeMap.fx", "VS", pVSBlob.Get());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\CubeMap.fx", "PS", pPSBlob.Get());

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.InputLayout = NXGlobalInputLayout::layoutP;
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<true, false, D3D12_COMPARISON_FUNC_LESS_EQUAL>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_pTexPassOut->GetFormat();
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));
}

void NXSkyRenderer::Init()
{
	m_pTexPassOut = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_SSSLighting);
	m_pTexPassOutDepth = NXResourceManager::GetInstance()->GetTextureManager()->GetCommonRT(NXCommonRT_DepthZ);

	NX12Util::CreateCommandQueue(NXGlobalDX::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT, false);
	NX12Util::CreateCommands(NXGlobalDX::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandQueue.Get(), m_pCommandAllocator.Get(), m_pCommandList.Get());

	InitSignature();
	InitPSO();
}

void NXSkyRenderer::Render()
{
	NX12Util::BeginEvent(m_pCommandList.Get(), "Sky (CubeMap IBL)");

	auto pShaderVisibleDescriptorHeap = NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorHeap();

	auto pCubeMap = m_pScene->GetCubeMap();
	if (pCubeMap)
	{
		m_pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
		m_pCommandList->SetPipelineState(m_pPSO.Get());

		ID3D12DescriptorHeap* ppHeaps[] = { pShaderVisibleDescriptorHeap->GetHeap() };
		m_pCommandList->SetDescriptorHeaps(1, ppHeaps);

		m_pTexPassOut->SetResourceState(m_pCommandList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_pTexPassOutDepth->SetResourceState(m_pCommandList.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

		auto rtvHandle = m_pTexPassOut->GetRTV();
		auto dsvHandle = m_pTexPassOutDepth->GetDSV();
		m_pCommandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = pShaderVisibleDescriptorHeap->Append(pCubeMap->GetSRVCubeMap());

		pCubeMap->UpdateViewParams();

		m_pCommandList->SetGraphicsRootConstantBufferView(0, NXGlobalBuffer::cbObject.Current().GPUVirtualAddr);
		m_pCommandList->SetGraphicsRootConstantBufferView(1, pCubeMap->GetCBDataParams());
		m_pCommandList->SetGraphicsRootDescriptorTable(2, srvHandle);

		pCubeMap->Render(m_pCommandList.Get());

		m_pTexPassOut->SetResourceState(m_pCommandList.Get(), D3D12_RESOURCE_STATE_COMMON);
		m_pTexPassOutDepth->SetResourceState(m_pCommandList.Get(), D3D12_RESOURCE_STATE_COMMON);
	}

	NX12Util::EndEvent();
}
