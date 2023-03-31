#include "NXBRDFlut.h"
#include "NXRenderStates.h"
#include "NXResourceManager.h"
#include "ShaderStructures.h"
#include "ShaderComplier.h"
#include "GlobalBufferManager.h"
#include "NXTexture.h"

NXBRDFLut::NXBRDFLut() :
	m_pTexBRDFLUT(nullptr)
{
}

void NXBRDFLut::GenerateBRDFLUT()
{
	g_pUDA->BeginEvent(L"Generate BRDF 2D LUT");

	NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::Create();

	const static float MapSize = 512.0f;
	CD3D11_VIEWPORT vp(0.0f, 0.0f, MapSize, MapSize);
	g_pContext->RSSetViewports(1, &vp);

	if (m_pTexBRDFLUT) m_pTexBRDFLUT->RemoveRef();
	m_pTexBRDFLUT = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("BRDF LUT", DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)MapSize, (UINT)MapSize, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, 1, 0, 0);
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

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(VertexPT) * (UINT)vertices.size();
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices.data();

	ComPtr<ID3D11Buffer> pVertexBuffer;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &pVertexBuffer));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(UINT) * (UINT)indices.size();
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	InitData.pSysMem = indices.data();

	ComPtr<ID3D11Buffer> pIndexBuffer;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &pIndexBuffer));

	ComPtr<ID3D11InputLayout> pInputLayoutPT;
	ComPtr<ID3D11VertexShader> pVertexShader;
	ComPtr<ID3D11PixelShader> pPixelShader;

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\BRDF2DLUT.fx", "VS", &pVertexShader, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &pInputLayoutPT);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\BRDF2DLUT.fx", "PS", &pPixelShader);
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

	g_pUDA->EndEvent();
}

void NXBRDFLut::Release()
{
	m_pTexBRDFLUT->RemoveRef();
}

ID3D11ShaderResourceView* NXBRDFLut::GetSRV()
{
	return m_pTexBRDFLUT->GetSRV();
}
