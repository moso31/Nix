#include "NXRenderTarget.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "GlobalBufferManager.h"

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

	g_pContext->IASetInputLayout(m_pInputLayout.Get());
	g_pContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	// QuadRTV的绘制只需要提供一下四边形顶点信息、SRV并绘制就好。
	// 这里可以做一个专用的QuadShader，然后无视掉indices，用Draw直接绘制Vertex而非DrawIndexed，可以获得更好的优化。但暂时没做。
	UINT stride = sizeof(VertexPT);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	auto pSRVMainScene = g_dxResources->GetSRVMainScene();
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

