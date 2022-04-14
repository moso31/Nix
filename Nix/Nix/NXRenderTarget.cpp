#include "NXRenderTarget.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "GlobalBufferManager.h"
#include "NXResourceManager.h"

NXRenderTarget::NXRenderTarget()
{
}

void NXRenderTarget::Init()
{
	float scale = 1.0f;
	// Create vertex buffer
	m_vertices =
	{
		// -Z
		{ Vector3(-scale, +scale, 0.0f), Vector2(0.0f, 0.0f) },
		{ Vector3(+scale, +scale, 0.0f), Vector2(1.0f, 0.0f) },
		{ Vector3(+scale, -scale, 0.0f), Vector2(1.0f, 1.0f) },
		{ Vector3(-scale, -scale, 0.0f), Vector2(0.0f, 1.0f) },
	};

	m_indices =
	{
		0,  1,  2,
		0,  2,  3
	};

	InitRenderData();
	InitVertexIndexBuffer();
}

void NXRenderTarget::Render()
{
	g_pUDA->BeginEvent(L"Render Target");

	// 以上操作全部都是在主RTV中进行的。
	// 下面切换到QuadRTV，简单来说就是将主RTV绘制到这个RTV，然后作为一张四边形纹理进行最终输出。
	ID3D11RenderTargetView* pRTVFinalQuad = g_dxResources->GetRTVFinalQuad();
	ID3D11ShaderResourceView* pSRVMainScene = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_MainScene)->GetSRV();
	ID3D11DepthStencilView* pDSVSceneDepth = NXResourceManager::GetInstance()->GetCommonRT(NXCommonRT_DepthZ)->GetDSV();

	g_pContext->OMSetRenderTargets(1, &pRTVFinalQuad, pDSVSceneDepth);
	g_pContext->ClearRenderTargetView(pRTVFinalQuad, Colors::Black);
	g_pContext->ClearDepthStencilView(pDSVSceneDepth, D3D11_CLEAR_DEPTH, 1.0f, 0);

	g_pContext->RSSetState(nullptr);
	g_pContext->OMSetDepthStencilState(nullptr, 0);

	g_pContext->IASetInputLayout(m_pInputLayout.Get());
	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	UINT stride = sizeof(VertexPT);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	g_pContext->PSSetShaderResources(0, 1, &pSRVMainScene);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);

	g_pUDA->EndEvent();
}

void NXRenderTarget::InitVertexIndexBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(VertexPT) * (UINT)m_vertices.size();
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = m_vertices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pVertexBuffer));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(UINT) * (UINT)m_indices.size();
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	InitData.pSysMem = m_indices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pIndexBuffer));
}

void NXRenderTarget::InitRenderData()
{
	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\RenderTarget.fx", "VS", &m_pVertexShader, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &m_pInputLayout);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\RenderTarget.fx", "PS", &m_pPixelShader);
}

