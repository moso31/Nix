#include "NXRenderTarget.h"
#include "DirectResources.h"

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
		{ Vector3(-scale, +scale, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f) },
		{ Vector3(+scale, +scale, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f) },
		{ Vector3(+scale, -scale, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f) },
		{ Vector3(-scale, -scale, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f) },
	};

	m_indices =
	{
		0,  1,  2,
		0,  2,  3
	};

	InitVertexIndexBuffer();
}

void NXRenderTarget::Render()
{
	// QuadRTV的绘制只需要提供一下四边形顶点信息、SRV并绘制就好。
	// 不过这里InputLayout还是用的VertexPNT格式，实际上做一个专用的VertexQuad顶点格式更加优化。这里没做。
	UINT stride = sizeof(VertexPNT);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	auto pRenderTargetSRV = g_dxResources->GetOffScreenSRV();
	g_pContext->PSSetShaderResources(0, 1, &pRenderTargetSRV);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);
}

