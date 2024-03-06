//#include "NXSimpleSSAO.h"
//#include "NXRandom.h"
//#include "ShaderComplier.h"
//#include "NXGlobalDefinitions.h"
//#include "DirectResources.h"
//#include "NXResourceManager.h"
//#include "NXRenderStates.h"
//#include "NXSamplerStates.h"
//#include "NXScene.h"
//#include "SamplerMath.h"
//#include "NXTexture.h"
//
//NXSimpleSSAO::NXSimpleSSAO()
//{
//}
//
//NXSimpleSSAO::~NXSimpleSSAO()
//{
//}
//
//void NXSimpleSSAO::Init()
//{
//	NXShaderComplier::GetInstance()->CompileCS(L"Shader\\SimpleSSAO.fx", "CS", &m_pComputeShader);
//
//	// SSAO Params
//	InitSSAOParams();
//
//	// ���������������
//	GenerateSamplePosition();
//}
//
//void NXSimpleSSAO::OnResize(const Vector2& rtSize)
//{
//	m_pTexSSAO = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("Simple SSAO", DXGI_FORMAT_R32G32B32A32_FLOAT, lround(rtSize.x), lround(rtSize.y), 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS);
//	m_pTexSSAO->AddSRV();
//	m_pTexSSAO->AddUAV();
//
//	m_rtSize = rtSize;
//}
//
//void NXSimpleSSAO::Update()
//{
//	g_pContext->UpdateSubresource(m_pCBSSAOParams.Get(), 0, nullptr, &m_ssaoParams, 0, 0);
//}
//
//void NXSimpleSSAO::Render(ID3D11ShaderResourceView* pSRVNormal, ID3D11ShaderResourceView* pSRVPosition, ID3D11ShaderResourceView* pSRVDepthPrepass)
//{
//	g_pUDA->BeginEvent(L"Simple SSAO");
//	g_pContext->CSSetShader(m_pComputeShader.Get(), nullptr, 0);
//	
//	g_pContext->CSSetConstantBuffers(0, 1, NXGlobalBuffer::cbCamera.GetAddressOf());
//	g_pContext->CSSetConstantBuffers(1, 1, m_pCBSamplePositions.GetAddressOf());
//	g_pContext->CSSetConstantBuffers(2, 1, m_pCBSSAOParams.GetAddressOf());
//
//	auto pSampler = NXSamplerManager::Get(NXSamplerFilter::Linear, NXSamplerAddressMode::Clamp);
//	g_pContext->CSSetSamplers(0, 1, &pSampler);
//
//	g_pContext->CSSetShaderResources(0, 1, &pSRVNormal);
//	g_pContext->CSSetShaderResources(1, 1, &pSRVPosition);
//	g_pContext->CSSetShaderResources(2, 1, &pSRVDepthPrepass);
//
//	auto pUAVSSAO = m_pTexSSAO->GetUAV();
//	g_pContext->CSSetUnorderedAccessViews(0, 1, &pUAVSSAO, nullptr);
//
//	int threadCountX = ((int)m_rtSize.x + 7) / 8;
//	int threadCountY = ((int)m_rtSize.y + 7) / 8;
//	g_pContext->Dispatch(threadCountX, threadCountY, 1);
//
//	// �����Ժ���ն�Ӧ��λ��SRV����Ȼ��һ֡����DepthPrepassʱDSV���ϡ�
//	ComPtr<ID3D11ShaderResourceView> pSRVNull[3] = { nullptr, nullptr, nullptr };
//	g_pContext->CSSetShaderResources(0, 3, pSRVNull->GetAddressOf());
//
//	ComPtr<ID3D11UnorderedAccessView> pUAVNull[1] = { nullptr };
//	g_pContext->CSSetUnorderedAccessViews(0, 1, pUAVNull->GetAddressOf(), nullptr);
//
//	g_pUDA->EndEvent();
//}
//
//ID3D11ShaderResourceView* NXSimpleSSAO::GetSRV()
//{
//	return m_pTexSSAO->GetSRV(); 
//}
//
//void NXSimpleSSAO::Release()
//{
//}
//
//void NXSimpleSSAO::InitSSAOParams()
//{
//	D3D11_BUFFER_DESC bufferDesc;
//	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
//	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
//	bufferDesc.ByteWidth = sizeof(ConstantBufferSSAOParams);
//	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
//	bufferDesc.CPUAccessFlags = 0;
//	NX::ThrowIfFailed(NXGlobalDX::GetDevice()->CreateBuffer(&bufferDesc, nullptr, &m_pCBSSAOParams));
//}
//
//void NXSimpleSSAO::GenerateSamplePosition()
//{
//	const static UINT SSAO_SAMPLE_COUNT = 256;
//	m_samplePosition.resize(SSAO_SAMPLE_COUNT);
//	for (int i = 0; i < SSAO_SAMPLE_COUNT; i++)
//	{
//		Vector2 u = NXRandom::GetInstance()->CreateVector2(0.0f, 1.0f);
//		float r = NXRandom::GetInstance()->CreateFloat(0.0f, 1.0f);
//		Vector3 v = SamplerMath::UniformSampleHemisphere(u, r);
//
//		m_samplePosition[i] = Vector4(v, 0.0f);
//	}
//
//	D3D11_BUFFER_DESC bufferDesc;
//	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
//	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
//	bufferDesc.ByteWidth = sizeof(Vector4) * SSAO_SAMPLE_COUNT;
//	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
//	bufferDesc.CPUAccessFlags = 0;
//	NX::ThrowIfFailed(NXGlobalDX::GetDevice()->CreateBuffer(&bufferDesc, nullptr, &m_pCBSamplePositions));
//
//	g_pContext->UpdateSubresource(m_pCBSamplePositions.Get(), 0, nullptr, m_samplePosition.data(), 0, 0);
//}
