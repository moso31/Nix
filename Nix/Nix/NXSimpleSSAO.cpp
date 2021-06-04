#include "NXSimpleSSAO.h"
#include "NXRandom.h"
#include "ShaderComplier.h"
#include "GlobalBufferManager.h"
#include "DirectResources.h"
#include "RenderStates.h"
#include "NXScene.h"
#include "SamplerMath.h"

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
		//DXGI_FORMAT_R32_FLOAT,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		lround(AOBufferSize.x),
		lround(AOBufferSize.y),
		1,
		1,
		D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS
	);
	g_pDevice->CreateTexture2D(&desc, nullptr, &pTexSSAO);
	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(pTexSSAO.Get(), nullptr, &m_pSRVSSAO));
	NX::ThrowIfFailed(g_pDevice->CreateUnorderedAccessView(pTexSSAO.Get(), nullptr, &m_pUAVSSAO));

	// 生成随机采样序列
	GenerateSamplePosition();
}

void NXSimpleSSAO::Render(ID3D11ShaderResourceView* pSRVNormal, ID3D11ShaderResourceView* pSRVPosition, ID3D11ShaderResourceView* pSRVDepthPrepass)
{
	g_pUDA->BeginEvent(L"Simple SSAO");
	g_pContext->CSSetShader(m_pComputeShader.Get(), nullptr, 0);
	
	g_pContext->CSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());
	g_pContext->CSSetConstantBuffers(1, 1, m_pCBSamplePositions.GetAddressOf());

	g_pContext->CSSetSamplers(0, 1, RenderStates::SamplerLinearClamp.GetAddressOf());
	g_pContext->CSSetShaderResources(0, 1, &pSRVNormal);
	g_pContext->CSSetShaderResources(1, 1, &pSRVPosition);
	g_pContext->CSSetShaderResources(2, 1, &pSRVDepthPrepass);
	g_pContext->CSSetUnorderedAccessViews(0, 1, m_pUAVSSAO.GetAddressOf(), nullptr);

	Vector2 screenSize = g_dxResources->GetViewPortSize();
	int threadCountX = ((int)screenSize.x + 7) / 8;
	int threadCountY = ((int)screenSize.y + 7) / 8;
	g_pContext->Dispatch(threadCountX, threadCountY, 1);

	// 用完以后清空对应槽位的SRV，不然下一帧处理DepthPrepass时DSV绑不上。
	ComPtr<ID3D11ShaderResourceView> pSRVNull[3] = { nullptr, nullptr, nullptr };
	g_pContext->CSSetShaderResources(0, 3, pSRVNull->GetAddressOf());

	g_pUDA->EndEvent();
}

void NXSimpleSSAO::GenerateSamplePosition()
{
	const static UINT SSAO_SAMPLE_COUNT = 256;
	m_samplePosition.resize(SSAO_SAMPLE_COUNT);
	for (int i = 0; i < SSAO_SAMPLE_COUNT; i++)
	{
		Vector2 u = NXRandom::GetInstance().CreateVector2(0.0f, 1.0f);
		float r = NXRandom::GetInstance().CreateFloat(0.0f, 1.0f);
		Vector3 v = SamplerMath::UniformSampleHemisphere(u, r);

		m_samplePosition[i] = Vector4(v, 0.0f);
	}

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(Vector4) * SSAO_SAMPLE_COUNT;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_pCBSamplePositions));

	g_pContext->UpdateSubresource(m_pCBSamplePositions.Get(), 0, nullptr, m_samplePosition.data(), 0, 0);
}
