#include "Renderer.h"
#include "DirectResources.h"
#include "ShaderComplier.h"
#include "WICTextureLoader.h"

HRESULT Renderer::InitRenderer()
{
	ID3DBlob* pVSBlob = nullptr;
	HRESULT hr = ShaderComplier::Compile(L"Tutorial03.fx", "VS", "vs_4_0", &pVSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the vertex shader
	hr = g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVertexShader);
	if (FAILED(hr))
	{
		pVSBlob->Release();
		return hr;
	}

	// Define the input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT numElements = ARRAYSIZE(layout);

	// Create the input layout
	hr = g_pDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
		pVSBlob->GetBufferSize(), &m_pInputLayout);
	pVSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Set the input layout
	g_pContext->IASetInputLayout(m_pInputLayout);

	// Compile the pixel shader
	ID3DBlob* pPSBlob = nullptr;
	hr = ShaderComplier::Compile(L"Tutorial03.fx", "PS", "ps_4_0", &pPSBlob);
	if (FAILED(hr))
	{
		MessageBox(nullptr,
			L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
		return hr;
	}

	// Create the pixel shader
	hr = g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPixelShader);
	pPSBlob->Release();
	if (FAILED(hr))
		return hr;

	// Create vertex buffer
	SimpleVertex vertices[] =
	{
		{ Vector3(-1.0f, 1.0f, -1.0f),		Vector2(1.0f, 0.0f) },
		{ Vector3(1.0f, 1.0f, -1.0f),		Vector2(0.0f, 0.0f) },
		{ Vector3(1.0f, 1.0f, 1.0f),		Vector2(0.0f, 1.0f) },
		{ Vector3(-1.0f, 1.0f, 1.0f),		Vector2(1.0f, 1.0f) },

		{ Vector3(-1.0f, -1.0f, -1.0f),		Vector2(0.0f, 0.0f) },
		{ Vector3(1.0f, -1.0f, -1.0f),		Vector2(1.0f, 0.0f) },
		{ Vector3(1.0f, -1.0f, 1.0f),		Vector2(1.0f, 1.0f) },
		{ Vector3(-1.0f, -1.0f, 1.0f),		Vector2(0.0f, 1.0f) },

		{ Vector3(-1.0f, -1.0f, 1.0f),		Vector2(0.0f, 1.0f) },
		{ Vector3(-1.0f, -1.0f, -1.0f),		Vector2(1.0f, 1.0f) },
		{ Vector3(-1.0f, 1.0f, -1.0f),		Vector2(1.0f, 0.0f) },
		{ Vector3(-1.0f, 1.0f, 1.0f),		Vector2(0.0f, 0.0f) },

		{ Vector3(1.0f, -1.0f, 1.0f),		Vector2(1.0f, 1.0f) },
		{ Vector3(1.0f, -1.0f, -1.0f),		Vector2(0.0f, 1.0f) },
		{ Vector3(1.0f, 1.0f, -1.0f),		Vector2(0.0f, 0.0f) },
		{ Vector3(1.0f, 1.0f, 1.0f),		Vector2(1.0f, 0.0f) },

		{ Vector3(-1.0f, -1.0f, -1.0f),		Vector2(0.0f, 1.0f) },
		{ Vector3(1.0f, -1.0f, -1.0f),		Vector2(1.0f, 1.0f) },
		{ Vector3(1.0f, 1.0f, -1.0f),		Vector2(1.0f, 0.0f) },
		{ Vector3(-1.0f, 1.0f, -1.0f),		Vector2(0.0f, 0.0f) },

		{ Vector3(-1.0f, -1.0f, 1.0f),		Vector2(1.0f, 1.0f) },
		{ Vector3(1.0f, -1.0f, 1.0f),		Vector2(0.0f, 1.0f) },
		{ Vector3(1.0f, 1.0f, 1.0f),		Vector2(0.0f, 0.0f) },
		{ Vector3(-1.0f, 1.0f, 1.0f),		Vector2(1.0f, 0.0f) },
	};

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(SimpleVertex) * 8;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices;
	hr = g_pDevice->CreateBuffer(&bd, &InitData, &m_pVertexBuffer);
	if (FAILED(hr))
		return hr;

	// Set vertex buffer
	UINT stride = sizeof(SimpleVertex);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);

	// Create index buffer
	WORD indices[] =
	{
		3, 1, 0,
		2, 1, 3,
		   	  
		0, 5, 4,
		1, 5, 0,
		   	  
		3, 4, 7,
		0, 4, 3,
		   	  
		1, 6, 5,
		2, 6, 1,
		   	  
		2, 7, 6,
		3, 7, 2,
		   	  
		6, 4, 5,
		7, 4, 6,
	};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(WORD) * 36;        // 36 vertices needed for 12 triangles in a triangle list
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;
	hr = g_pDevice->CreateBuffer(&bd, &InitData, &m_pIndexBuffer);
	if (FAILED(hr))
		return hr;

	g_pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	hr = g_pDevice->CreateBuffer(&bd, nullptr, &m_pConstantBuffer);
	if (FAILED(hr))
		return hr;

	hr = CreateWICTextureFromFile(g_pDevice, L"D:\\rgb.bmp", nullptr, &m_pBoxSRV);
	if (FAILED(hr))
		return hr;

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pDevice->CreateSamplerState(&sampDesc, &m_pSamplerLinear);
	if (FAILED(hr))
		return hr;

	// Initialize the world matrix
	m_boxData.mWorld = XMMatrixIdentity();

	// Initialize the view matrix
	Vector4 Eye(0.0f, 1.0f, -5.0f, 0.0f);
	Vector4 At(0.0f, 1.0f, 0.0f, 0.0f);
	Vector4 Up(0.0f, 1.0f, 0.0f, 0.0f);
	m_boxData.mView = XMMatrixLookAtLH(Eye, At, Up);

	// Initialize the projection matrix
	Vector2 vpsz = g_dxResources->GetViewPortSize();
	m_boxData.mProjection = XMMatrixPerspectiveFovLH(XM_PIDIV2, vpsz.x / vpsz.y, 0.01f, 100.0f);

	return S_OK;
}

void Renderer::Update()
{
	// Update our time
	static float t = 0.0f;
	static ULONGLONG timeStart = 0;
	ULONGLONG timeCur = GetTickCount64();
	if (timeStart == 0)
		timeStart = timeCur;
	t = (timeCur - timeStart) / 1000.0f;

	m_boxData.mWorld = XMMatrixRotationY(t);

	ConstantBuffer cb;
	cb.mWorld = XMMatrixTranspose(m_boxData.mWorld);
	cb.mView = XMMatrixTranspose(m_boxData.mView);
	cb.mProjection = XMMatrixTranspose(m_boxData.mProjection);
	g_pContext->UpdateSubresource(m_pConstantBuffer, 0, nullptr, &cb, 0, 0);
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
	g_pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	g_pContext->PSSetShader(m_pPixelShader, nullptr, 0);
	g_pContext->PSSetShaderResources(0, 1, &m_pBoxSRV);
	g_pContext->PSSetSamplers(0, 1, &m_pSamplerLinear);
	g_pContext->DrawIndexed(36, 0, 0);

	// Present the information rendered to the back buffer to the front buffer (the screen)
	g_pSwapChain->Present(0, 0);
}

void Renderer::Release()
{
	if (m_pInputLayout)		m_pInputLayout->Release();
	if (m_pVertexShader)	m_pVertexShader->Release();
	if (m_pPixelShader)		m_pPixelShader->Release();
	if (m_pVertexBuffer)	m_pVertexBuffer->Release();
	if (m_pIndexBuffer)		m_pIndexBuffer->Release();
	if (m_pConstantBuffer)	m_pConstantBuffer->Release();
	if (m_pBoxSRV)			m_pBoxSRV->Release();
	if (m_pSamplerLinear)	m_pSamplerLinear->Release();
}
