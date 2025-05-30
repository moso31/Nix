#include "NXBRDFlut.h"
#include "NXSamplerManager.h"
#include "NXResourceManager.h"
#include "ShaderStructures.h"
#include "ShaderComplier.h"
#include "NXGlobalDefinitions.h"
#include "NXTexture.h"
#include "NXRenderStates.h"
#include "NXSubMeshGeometryEditor.h"

NXBRDFLut::NXBRDFLut() :
	m_subMesh(nullptr, "_PlanePositiveZ")
{
}

void NXBRDFLut::Init()
{
	m_pFence = NX12Util::CreateFence(NXGlobalDX::GetDevice(), L"Create fence FAILED in NXBRDFLut.");
	NX12Util::CreateCommands(NXGlobalDX::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandQueue.GetAddressOf(), m_pCommandAllocator.GetAddressOf(), m_pCommandList.GetAddressOf());
	m_pCommandQueue->SetName(L"BRDF LUT Command Queue");

	m_pTexBRDFLUT = NXResourceManager::GetInstance()->GetTextureManager()->CreateRenderTexture("BRDF LUT", DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)m_mapSize, (UINT)m_mapSize, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

	InitVertex();
	InitRootSignature();
	DrawBRDFLUT();
}

void NXBRDFLut::Release()
{
}

ID3D12Fence* NXBRDFLut::GlobalFence()
{
	return m_pFence.Get();
}

uint64_t NXBRDFLut::GetFenceValue()
{
	return m_fenceValue;
}

void NXBRDFLut::InitVertex()
{
	m_subMesh.AppendVertices(
	{
		// +Z
		{ Vector3(+1.0f, +1.0f, +1.0f), Vector2(1.0f, 1.0f) },
		{ Vector3(-1.0f, +1.0f, +1.0f), Vector2(1.0f, 0.0f) },
		{ Vector3(-1.0f, -1.0f, +1.0f), Vector2(0.0f, 0.0f) },
		{ Vector3(+1.0f, -1.0f, +1.0f), Vector2(0.0f, 1.0f) },
	});

	m_subMesh.AppendIndices(
	{
		0,  2,	1,
		0,  3,	2,
	});

	m_subMesh.TryAddBuffers();
}

void NXBRDFLut::InitRootSignature()
{
	ComPtr<IDxcBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\BRDF2DLUT.fx", L"VS", pVSBlob.GetAddressOf());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\BRDF2DLUT.fx", L"PS", pPSBlob.GetAddressOf());

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
	m_pTexBRDFLUT->WaitLoadingViewsFinish();

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
	D3D12_VERTEX_BUFFER_VIEW vbv;
	if (meshView.GetVBV(0, vbv))
		m_pCommandList->IASetVertexBuffers(0, 1, &vbv);

	D3D12_INDEX_BUFFER_VIEW ibv;
	if (meshView.GetIBV(1, ibv))
		m_pCommandList->IASetIndexBuffer(&ibv);

	m_pCommandList->DrawIndexedInstanced(meshView.GetIndexCount(), 1, 0, 0, 0);

	m_pTexBRDFLUT->SetResourceState(m_pCommandList.Get(), D3D12_RESOURCE_STATE_COMMON);

	NX12Util::EndEvent(m_pCommandList.Get());

	m_pCommandList->Close();
	ID3D12CommandList* pCmdLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(1, pCmdLists);

	m_fenceValue++;
	m_pCommandQueue->Signal(m_pFence.Get(), m_fenceValue);

	// 等待围栏完成
	if (m_pFence->GetCompletedValue() < m_fenceValue)
	{
		HANDLE fenceEvent = CreateEvent(nullptr, false, false, nullptr);
		m_pFence->SetEventOnCompletion(m_fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);  // 等待围栏信号完成
		CloseHandle(fenceEvent);
	}
}
