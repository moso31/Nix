#include "NXBRDFlut.h"
#include "NXSamplerStates.h"
#include "NXResourceManager.h"
#include "ShaderStructures.h"
#include "ShaderComplier.h"
#include "GlobalBufferManager.h"
#include "NXTexture.h"
#include "NXSubMeshGeometryEditor.h"

NXBRDFLut::NXBRDFLut() 
{
}

void NXBRDFLut::Init()
{
	InitVertex();
	InitRoogSignature();
	DrawBRDFLUT();
}

void NXBRDFLut::Release()
{
}

void NXBRDFLut::InitVertex()
{
	m_pTexBRDFLUT = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("BRDF LUT", DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)MapSize, (UINT)MapSize, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	m_pTexBRDFLUT->AddSRV();
	m_pTexBRDFLUT->AddRTV();

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
	ComPtr<ID3DBlob> pBlobVS, pBlobPS;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\BRDF2DLUT.fx", "VS", pBlobVS.Get());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\BRDF2DLUT.fx", "PS", pBlobPS.Get());

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSig.Get();
	psoDesc.InputLayout = { NXGlobalInputLayout::layoutPT, 1 };
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = pTexCubeMap->GetFormat();
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	g_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSOCubeMap));
}

void NXBRDFLut::DrawBRDFLUT()
{
	NX12Util::BeginEvent(m_pCommandList.Get(), "Generate BRDF 2D LUT");

	const static float MapSize = 512.0f;
	auto vp = NX12Util::ViewPort(MapSize, MapSize);
	m_pCommandList->RSSetViewports(1, &vp);

	g_pContext->VSSetShader(pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(pInputLayoutPT.Get());

	UINT stride = sizeof(VertexPT);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &stride, &offset);
	g_pContext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	auto pRTV = m_pTexBRDFLUT->GetRTV();
	g_pContext->ClearRenderTargetView(pRTV, Colors::WhiteSmoke);
	g_pContext->OMSetRenderTargets(1, &pRTV, nullptr);
	g_pContext->DrawIndexed((UINT)indices.size(), 0, 0);

	NX12Util::EndEvent();
}

ID3D11ShaderResourceView* NXBRDFLut::GetSRV()
{
	return m_pTexBRDFLUT->GetSRV();
}
