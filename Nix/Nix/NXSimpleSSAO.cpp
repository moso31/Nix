#include "NXSimpleSSAO.h"
#include "ShaderComplier.h"
#include "GlobalBufferManager.h"
#include "DirectResources.h"
#include "NXScene.h"

NXSimpleSSAO::NXSimpleSSAO()
{
}

NXSimpleSSAO::~NXSimpleSSAO()
{
}

void NXSimpleSSAO::Init(const Vector2& AOBufferSize)
{
	// create VS & IL
	ComPtr<ID3DBlob> pCSBlob;
	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\SimpleSSAO.fx", "CS", "cs_5_0", &pCSBlob),
		L"[NXSimpleSSAO compile failed]. Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateComputeShader(pCSBlob->GetBufferPointer(), pCSBlob->GetBufferSize(), nullptr, &m_pComputeShader));

	ComPtr<ID3D11Texture2D> pTexSSAO;
	CD3D11_TEXTURE2D_DESC desc(
		DXGI_FORMAT_R32_FLOAT,
		lround(AOBufferSize.x),
		lround(AOBufferSize.y),
		1,
		1,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
	);
	g_pDevice->CreateTexture2D(&desc, nullptr, &pTexSSAO);
	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(pTexSSAO.Get(), nullptr, &m_pSRVSSAO));
	NX::ThrowIfFailed(g_pDevice->CreateUnorderedAccessView(pTexSSAO.Get(), nullptr, &m_pUAVSSAO));
}

void NXSimpleSSAO::Render(ID3D11ShaderResourceView* pSRVNormal, ID3D11ShaderResourceView* pSRVDepthPrepass)
{
	g_pUDA->BeginEvent(L"Simple SSAO");
	g_pContext->CSSetShader(m_pComputeShader.Get(), nullptr, 0);

	g_pContext->CSSetShaderResources(0, 1, &pSRVNormal);
	g_pContext->CSSetShaderResources(1, 1, &pSRVDepthPrepass);
	g_pContext->CSSetUnorderedAccessViews(0, 1, &m_pUAVSSAO, nullptr);

	Vector2 screenSize = g_dxResources->GetViewPortSize();
	int threadCountX = ((int)screenSize.x + 7) / 8;
	int threadCountY = ((int)screenSize.y + 7) / 8;
	g_pContext->Dispatch(threadCountX, threadCountY, 1);

	// 用完以后清空pSRVDepthStencil对应槽位的SRV，不然下一帧处理DepthPrepass时DSV绑不上。
	ID3D11ShaderResourceView* pSRVNull = nullptr;
	g_pContext->CSSetShaderResources(1, 1, &pSRVNull);

	g_pUDA->EndEvent();
}
