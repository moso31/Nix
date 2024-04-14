#include "NXBRDFlut.h"
#include "NXSamplerManager.h"
#include "NXResourceManager.h"
#include "ShaderStructures.h"
#include "ShaderComplier.h"
#include "NXGlobalDefinitions.h"
#include "NXTexture.h"
#include "NXRenderStates.h"
#include "NXSubMeshGeometryEditor.h"

NXBRDFLut::NXBRDFLut() 
{
}

void NXBRDFLut::Init()
{
	NX12Util::CreateCommands(NXGlobalDX::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandQueue.GetAddressOf(), m_pCommandAllocator.GetAddressOf(), m_pCommandList.GetAddressOf());

	m_pTexBRDFLUT = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("BRDF LUT", DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)m_mapSize, (UINT)m_mapSize, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pTexBRDFLUT->AddSRV();
	m_pTexBRDFLUT->AddRTV();

	InitVertex();
	InitRootSignature();
	DrawBRDFLUT();
}

void NXBRDFLut::Release()
{
}

void NXBRDFLut::InitVertex()
{
	std::vector<VertexPT> vertices =
	{
		// +Z
		{ Vector3(+1.0f, +1.0f, +1.0f), Vector2(1.0f, 1.0f) },
		{ Vector3(-1.0f, +1.0f, +1.0f), Vector2(1.0f, 0.0f) },
		{ Vector3(-1.0f, -1.0f, +1.0f), Vector2(0.0f, 0.0f) },
		{ Vector3(+1.0f, -1.0f, +1.0f), Vector2(0.0f, 1.0f) },
	};

	std::vector<UINT> indices =
	{
		0,  2,	1,
		0,  3,	2,
	};

	NXSubMeshGeometryEditor::GetInstance()->CreateVBIB(vertices, indices, "_PlanePositiveZ");
}

void NXBRDFLut::InitRootSignature()
{
	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\BRDF2DLUT.fx", "VS", pVSBlob.GetAddressOf());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\BRDF2DLUT.fx", "PS", pPSBlob.GetAddressOf());

	m_pRootSig = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), 0, nullptr, 0, nullptr);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.InputLayout = NXGlobalInputLayout::layoutPT;
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<false>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_pTexBRDFLUT->GetFormat();
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSO));
}

void NXBRDFLut::DrawBRDFLUT()
{
	m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr);

	NX12Util::BeginEvent(m_pCommandList.Get(), "Generate BRDF 2D LUT");

	auto vp = NX12Util::ViewPort(m_mapSize, m_mapSize);
	m_pCommandList->RSSetViewports(1, &vp);
	m_pCommandList->RSSetScissorRects(1, &NX12Util::ScissorRect(vp));

	m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_pCommandList->SetGraphicsRootSignature(m_pRootSig.Get());
	m_pCommandList->SetPipelineState(m_pPSO.Get());

	m_pTexBRDFLUT->SetResourceState(m_pCommandList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_pCommandList->OMSetRenderTargets(1, &m_pTexBRDFLUT->GetRTV(), false, nullptr);

	const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews("_PlanePositiveZ");
	m_pCommandList->IASetVertexBuffers(0, 1, &meshView.vbv);
	m_pCommandList->IASetIndexBuffer(&meshView.ibv);
	m_pCommandList->DrawIndexedInstanced(meshView.indexCount, 1, 0, 0, 0);

	m_pTexBRDFLUT->SetResourceState(m_pCommandList.Get(), D3D12_RESOURCE_STATE_COMMON);

	NX12Util::EndEvent();

	m_pCommandList->Close();
	ID3D12CommandList* pCmdLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(1, pCmdLists);
}
