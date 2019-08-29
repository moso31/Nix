#include "Renderer.h"
#include "DirectResources.h"
#include "ShaderComplier.h"

#include "NXScene.h"

void Renderer::Init()
{
	InitRenderer();

	m_scene = make_shared<Scene>();
	m_scene->Init();
}

void Renderer::InitRenderer()
{
	ID3DBlob* pVSBlob = nullptr;
	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\Tutorial03.fx", "VS", "vs_4_0", &pVSBlob), 
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");

	// Create the vertex shader
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader));

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayout));
	pVSBlob->Release();

	// Set the input layout
	g_pContext->IASetInputLayout(m_pInputLayout);

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\Tutorial03.fx", "PS", "ps_4_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");

	// Create the pixel shader
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader));
	pPSBlob->Release();

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	NX::ThrowIfFailed(g_pDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinear));
}

void Renderer::Update()
{
	m_scene->PrevUpdate();
	m_scene->Update();
}

void Renderer::Render()
{
	auto pRenderTargetView = g_dxResources->GetRenderTargetView();
	auto pDepthStencilView = g_dxResources->GetDepthStencilView();
	g_pContext->ClearRenderTargetView(pRenderTargetView, Colors::WhiteSmoke);
	g_pContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	g_pContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);

	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Render a triangle
	g_pContext->VSSetShader(m_pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(m_pPixelShader, nullptr, 0);
	g_pContext->PSSetSamplers(0, 1, &m_pSamplerLinear);

	m_scene->Render();

	DXGI_PRESENT_PARAMETERS parameters = { 0 };
	HRESULT hr = g_pSwapChain->Present1(1, 0, &parameters);
}

void Renderer::Release()
{
	if (m_pInputLayout)		m_pInputLayout->Release();
	if (m_pVertexShader)	m_pVertexShader->Release();
	if (m_pPixelShader)		m_pPixelShader->Release();
	if (m_pSamplerLinear)	m_pSamplerLinear->Release();

	m_scene.reset();
}
