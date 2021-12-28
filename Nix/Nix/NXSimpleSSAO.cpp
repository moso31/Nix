#include "NXSimpleSSAO.h"
#include "NXRandom.h"
#include "ShaderComplier.h"
#include "GlobalBufferManager.h"
#include "DirectResources.h"
#include "NXResourceManager.h"
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

	m_pTexSSAO = NXResourceManager::GetInstance()->CreateTexture2D("Simple SSAO", DXGI_FORMAT_R32G32B32A32_FLOAT, lround(AOBufferSize.x), lround(AOBufferSize.y), 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
	m_pTexSSAO->CreateSRV();
	m_pTexSSAO->CreateUAV();

	// SSAO Params
	InitSSAOParams();

	// 生成随机采样序列
	GenerateSamplePosition();
}

void NXSimpleSSAO::Update()
{
	g_pContext->UpdateSubresource(m_pCBSSAOParams.Get(), 0, nullptr, &m_ssaoParams, 0, 0);
}

void NXSimpleSSAO::Render(ID3D11ShaderResourceView* pSRVNormal, ID3D11ShaderResourceView* pSRVPosition, ID3D11ShaderResourceView* pSRVDepthPrepass)
{
	g_pUDA->BeginEvent(L"Simple SSAO");
	g_pContext->CSSetShader(m_pComputeShader.Get(), nullptr, 0);
	
	g_pContext->CSSetConstantBuffers(0, 1, NXGlobalBufferManager::m_cbCamera.GetAddressOf());
	g_pContext->CSSetConstantBuffers(1, 1, m_pCBSamplePositions.GetAddressOf());
	g_pContext->CSSetConstantBuffers(2, 1, m_pCBSSAOParams.GetAddressOf());

	g_pContext->CSSetSamplers(0, 1, RenderStates::SamplerLinearClamp.GetAddressOf());
	g_pContext->CSSetShaderResources(0, 1, &pSRVNormal);
	g_pContext->CSSetShaderResources(1, 1, &pSRVPosition);
	g_pContext->CSSetShaderResources(2, 1, &pSRVDepthPrepass);

	auto pUAVSSAO = m_pTexSSAO->GetUAV();
	g_pContext->CSSetUnorderedAccessViews(0, 1, &pUAVSSAO, nullptr);

	Vector2 screenSize = g_dxResources->GetViewPortSize();
	int threadCountX = ((int)screenSize.x + 7) / 8;
	int threadCountY = ((int)screenSize.y + 7) / 8;
	g_pContext->Dispatch(threadCountX, threadCountY, 1);

	// 用完以后清空对应槽位的SRV，不然下一帧处理DepthPrepass时DSV绑不上。
	ComPtr<ID3D11ShaderResourceView> pSRVNull[3] = { nullptr, nullptr, nullptr };
	g_pContext->CSSetShaderResources(0, 3, pSRVNull->GetAddressOf());

	ComPtr<ID3D11UnorderedAccessView> pUAVNull[1] = { nullptr };
	g_pContext->CSSetUnorderedAccessViews(0, 1, pUAVNull->GetAddressOf(), nullptr);

	g_pUDA->EndEvent();
}

ID3D11ShaderResourceView* NXSimpleSSAO::GetSRV()
{
	return m_pTexSSAO->GetSRV(); 
}

void NXSimpleSSAO::Release()
{
	SafeDelete(m_pTexSSAO);
}

void NXSimpleSSAO::InitSSAOParams()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferSSAOParams);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_pCBSSAOParams));
}

void NXSimpleSSAO::GenerateSamplePosition()
{
	const static UINT SSAO_SAMPLE_COUNT = 256;
	m_samplePosition.resize(SSAO_SAMPLE_COUNT);
	for (int i = 0; i < SSAO_SAMPLE_COUNT; i++)
	{
		Vector2 u = NXRandom::GetInstance()->CreateVector2(0.0f, 1.0f);
		float r = NXRandom::GetInstance()->CreateFloat(0.0f, 1.0f);
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
