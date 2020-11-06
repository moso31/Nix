#include "NXCubeMap.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXCamera.h"
#include "ShaderComplier.h"
#include "DirectResources.h"

NXCubeMap::NXCubeMap(NXScene* pScene) :
	m_pScene(pScene),
	m_pEnvironmentLight(nullptr),
	m_pTexCubeMap(nullptr),
	m_pSRVCubeMap(nullptr),
	m_pTexIrradianceMap(nullptr),
	m_pSRVIrradianceMap(nullptr),
	m_pTexPreFilterMap(nullptr),
	m_pSRVPreFilterMap(nullptr),
	m_pTexBRDF2DLUT(nullptr),
	m_pSRVBRDF2DLUT(nullptr),
	m_pRTVBRDF2DLUT(nullptr)
{
	for (int i = 0; i < 6; i++) m_faceData[i] = nullptr;
	for (int i = 0; i < 6; i++) m_pRTVIrradianceMaps[i] = nullptr;
	for (int i = 0; i < 5; i++)
		for (int j = 0; j < 6; j++)
			m_pRTVPreFilterMaps[i][j] = nullptr;
}

bool NXCubeMap::Init(std::wstring filePath)
{
	m_image.reset(); 
	m_image = std::make_unique<ScratchImage>();

	TexMetadata metadata;
	std::unique_ptr<ScratchImage> dcImage = std::make_unique<ScratchImage>();
	HRESULT hr; 
	hr = LoadFromDDSFile(filePath.c_str(), DDS_FLAGS_NONE, &metadata, *m_image);
	if (FAILED(hr))
	{
		dcImage.reset();
		return false;
	}

	// 创建CubeMap SRV。
	// 需要使用初始格式创建，也就是说要在Decompress之前创建。
	CreateTextureEx(g_pDevice, m_image->GetImages(), m_image->GetImageCount(), metadata, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, false, (ID3D11Resource**)&m_pTexCubeMap);
	CreateShaderResourceView(g_pDevice, m_image->GetImages(), m_image->GetImageCount(), metadata, &m_pSRVCubeMap);

	if (IsCompressed(metadata.format))
	{
		auto img = m_image->GetImage(0, 0, 0);
		size_t nimg = m_image->GetImageCount();

		hr = Decompress(img, nimg, metadata, DXGI_FORMAT_UNKNOWN /* picks good default */, *dcImage);
		if (FAILED(hr))
		{
			dcImage.reset();
			return false;
		}
	}
	if (dcImage) m_image.swap(dcImage);
	dcImage.reset();

	if (!m_image) return false;

	m_width = metadata.width;
	m_height = metadata.height;

	for (int item = 0; item < 6; item++)
	{
		auto faceImage = m_image->GetImage(0, item, 0);
		m_faceData[item] = faceImage->pixels;
	}

	// create vertex
	InitVertex();

	return true;
}

Vector3 NXCubeMap::BackgroundColorByDirection(const Vector3& v)
{
	assert(v.LengthSquared() != 0);

	Vector3 vAbs = Vector3::Abs(v);
	int dim = vAbs.MaxDimension();
	float scale = 1.0f / vAbs[dim];
	Vector3 touchCube = v * scale;	// 将v延长到能摸到单位Cube。此时dim轴即被选中的CubeMap的面，剩下的两个轴即=该面的uv。
	touchCube = (touchCube + Vector3(1.0f)) * 0.5f; //将坐标从([-1, 1]^3)映射到([0, 1]^3)

	assert(dim >= 0 && dim < 3);
	Vector2 uvHit;

	int faceId = dim * 2 + !(v[dim] > 0);	
	switch (faceId)
	{
	case 0:		uvHit = Vector2(1.0f - touchCube.z, 1.0f - touchCube.y);	break;		// +X
	case 1:		uvHit = Vector2(touchCube.z, 1.0f - touchCube.y);	break;		// -X
	case 2:		uvHit = Vector2(touchCube.x, touchCube.z);	break;		// +Y
	case 3:		uvHit = Vector2(touchCube.z, touchCube.x);	break;		// -Y
	case 4:		uvHit = Vector2(touchCube.x, 1.0f - touchCube.y);	break;		// +Z
	case 5:		uvHit = Vector2(1.0f - touchCube.x, 1.0f - touchCube.y);	break;		// -Z
	}

	int offset = ((int)floorf(uvHit.y * (float)m_height) * (int)m_width + (int)floorf(uvHit.x * (float)m_width)) * 4;
	byte* c = m_faceData[faceId] + offset;

	float fInv = 1.0f / 255.0f;
	float r = *(c + 0) * fInv;
	float g = *(c + 1) * fInv;
	float b = *(c + 2) * fInv;
	return Vector3(r, g, b);
}

void NXCubeMap::Update()
{
	auto pCamera = m_pScene->GetMainCamera();
	NXGlobalBufferManager::m_cbDataObject.world = Matrix::CreateTranslation(pCamera->GetTranslation()).Transpose();
	g_pContext->UpdateSubresource(NXGlobalBufferManager::m_cbObject, 0, nullptr, &NXGlobalBufferManager::m_cbDataObject, 0, 0);
}

void NXCubeMap::Render()
{
	UINT stride = sizeof(VertexP);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);
}

void NXCubeMap::Release()
{
	if (m_pTexCubeMap) m_pTexCubeMap->Release();
	if (m_pSRVCubeMap) m_pSRVCubeMap->Release();

	if (m_pTexIrradianceMap) m_pTexIrradianceMap->Release();
	if (m_pSRVIrradianceMap) m_pSRVIrradianceMap->Release();
	for (int i = 0; i < 6; i++)
		if (m_pRTVIrradianceMaps[i]) m_pRTVIrradianceMaps[i]->Release();

	if (m_pTexPreFilterMap) m_pTexPreFilterMap->Release();
	if (m_pSRVPreFilterMap) m_pSRVPreFilterMap->Release();
	for (int i = 0; i < 5; i++)
		for (int j = 0; j < 6; j++)
			if (m_pRTVPreFilterMaps[i][j]) m_pRTVPreFilterMaps[i][j]->Release();

	if (m_pTexBRDF2DLUT) m_pTexBRDF2DLUT->Release();
	if (m_pSRVBRDF2DLUT) m_pSRVBRDF2DLUT->Release();
	if (m_pRTVBRDF2DLUT) m_pRTVBRDF2DLUT->Release();

	if (m_image) m_image.reset();
	for (int i = 0; i < 6; i++) m_faceData[i] = nullptr;
	NXPrimitive::Release();
}

void NXCubeMap::GenerateIrradianceMap()
{
	const static float MapSize = 32.0f;
	CD3D11_VIEWPORT vp(0.0f, 0.0f, MapSize, MapSize);
	g_pContext->RSSetViewports(1, &vp);

	CD3D11_TEXTURE2D_DESC descTex(DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)MapSize, (UINT)MapSize, 6, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, 1, 0, D3D11_RESOURCE_MISC_TEXTURECUBE);
	g_pDevice->CreateTexture2D(&descTex, nullptr, &m_pTexIrradianceMap);

	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(D3D11_SRV_DIMENSION_TEXTURECUBE, descTex.Format, 0, descTex.MipLevels, 0, descTex.ArraySize);
	g_pDevice->CreateShaderResourceView(m_pTexIrradianceMap, &descSRV, &m_pSRVIrradianceMap);

	for (int i = 0; i < 6; i++)
	{
		CD3D11_RENDER_TARGET_VIEW_DESC descRTV(D3D11_RTV_DIMENSION_TEXTURE2DARRAY, descTex.Format, 0, i, 1);
		g_pDevice->CreateRenderTargetView(m_pTexIrradianceMap, &descRTV, &m_pRTVIrradianceMaps[i]);
	}

	std::vector<VertexP> vertices =
	{
		// +X
		{ Vector3(+0.5f, +0.5f, -0.5f) },
		{ Vector3(+0.5f, +0.5f, +0.5f) },
		{ Vector3(+0.5f, -0.5f, +0.5f) },
		{ Vector3(+0.5f, -0.5f, -0.5f) },

		// -X
		{ Vector3(-0.5f, +0.5f, +0.5f) },
		{ Vector3(-0.5f, +0.5f, -0.5f) },
		{ Vector3(-0.5f, -0.5f, -0.5f) },
		{ Vector3(-0.5f, -0.5f, +0.5f) },

		// +Y
		{ Vector3(-0.5f, +0.5f, +0.5f) },
		{ Vector3(+0.5f, +0.5f, +0.5f) },
		{ Vector3(+0.5f, +0.5f, -0.5f) },
		{ Vector3(-0.5f, +0.5f, -0.5f) },

		// -Y
		{ Vector3(-0.5f, -0.5f, -0.5f) },
		{ Vector3(+0.5f, -0.5f, -0.5f) },
		{ Vector3(+0.5f, -0.5f, +0.5f) },
		{ Vector3(-0.5f, -0.5f, +0.5f) },

		// +Z
		{ Vector3(+0.5f, +0.5f, +0.5f) },
		{ Vector3(-0.5f, +0.5f, +0.5f) },
		{ Vector3(-0.5f, -0.5f, +0.5f) },
		{ Vector3(+0.5f, -0.5f, +0.5f) },

		// -Z
		{ Vector3(-0.5f, +0.5f, -0.5f) },
		{ Vector3(+0.5f, +0.5f, -0.5f) },
		{ Vector3(+0.5f, -0.5f, -0.5f) },
		{ Vector3(-0.5f, -0.5f, -0.5f) },
	};

	std::vector<USHORT> indices =
	{
		0,  2,	1,
		0,  3,	2,

		4,  6,	5,
		4,  7,	6,

		8,  10,	9,
		8,  11,	10,

		12, 14,	13,
		12, 15,	14,

		16, 18,	17,
		16, 19,	18,

		20, 22,	21,
		20, 23,	22,
	};

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(VertexP) * (UINT)vertices.size();
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices.data();

	ID3D11Buffer* pVertexBuffer;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &pVertexBuffer));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(USHORT) * (UINT)indices.size();
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	InitData.pSysMem = indices.data();

	ID3D11Buffer* pIndexBuffer;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &pIndexBuffer));

	ID3D11InputLayout* pInputLayoutPNT;
	ID3D11VertexShader* pVertexShader;
	ID3D11PixelShader* pPixelShader;
	ID3DBlob* pVSBlob = nullptr;
	ID3DBlob* pPSBlob = nullptr;

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\CubeMapIrradiance.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &pVertexShader));

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPNT, ARRAYSIZE(NXGlobalInputLayout::layoutPNT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &pInputLayoutPNT));
	g_pContext->IASetInputLayout(pInputLayoutPNT);

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\CubeMapIrradiance.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pPixelShader));

	pVSBlob->Release();
	pPSBlob->Release();

	ID3D11Buffer* cb = nullptr;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferObject);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &cb));

	Matrix mxCubeMapProj = XMMatrixPerspectiveFovLH(XMConvertToRadians(90.0f), 1.0f, 0.1f, 10.0f);
	Matrix mxCubeMapView[6] =
	{
		XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 1.0f,  0.0f,  0.0f), Vector3(0.0f, 1.0f,  0.0f)),
		XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3(-1.0f,  0.0f,  0.0f), Vector3(0.0f, 1.0f,  0.0f)),
		XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 0.0f,  1.0f,  0.0f), Vector3(0.0f, 0.0f, -1.0f)),
		XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 0.0f, -1.0f,  0.0f), Vector3(0.0f, 0.0f,  1.0f)),
		XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 0.0f,  0.0f,  1.0f), Vector3(0.0f, 1.0f,  0.0f)),
		XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 0.0f,  0.0f, -1.0f), Vector3(0.0f, 1.0f,  0.0f))
	};

	g_pContext->VSSetShader(pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(pPixelShader, nullptr, 0);
	g_pContext->PSSetShaderResources(0, 1, &m_pSRVCubeMap);
	g_pContext->VSSetConstantBuffers(0, 1, &cb);

	ConstantBufferObject cbData;
	cbData.world = Matrix::Identity();
	cbData.projection = mxCubeMapProj.Transpose();

	UINT stride = sizeof(VertexP);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	for (int i = 0; i < 6; i++)
	{
		g_pContext->ClearRenderTargetView(m_pRTVIrradianceMaps[i], Colors::WhiteSmoke);
		g_pContext->OMSetRenderTargets(1, &m_pRTVIrradianceMaps[i], nullptr);

		cbData.view = mxCubeMapView[i].Transpose();
		g_pContext->UpdateSubresource(cb, 0, nullptr, &cbData, 0, 0);
		g_pContext->DrawIndexed((UINT)indices.size() / 6, i * 6, 0);
	}

	pVertexBuffer->Release();
	pIndexBuffer->Release();
	pVertexShader->Release();
	pPixelShader->Release();
	pInputLayoutPNT->Release();
	cb->Release();
}

void NXCubeMap::GeneratePreFilterMap()
{
	const static float MapSize = 512.0f;
	CD3D11_TEXTURE2D_DESC descTex(DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)MapSize, (UINT)MapSize, 6, 5, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, 1, 0, D3D11_RESOURCE_MISC_TEXTURECUBE);
	g_pDevice->CreateTexture2D(&descTex, nullptr, &m_pTexPreFilterMap);

	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(D3D11_SRV_DIMENSION_TEXTURECUBE, descTex.Format, 0, descTex.MipLevels, 0, descTex.ArraySize);
	g_pDevice->CreateShaderResourceView(m_pTexPreFilterMap, &descSRV, &m_pSRVPreFilterMap);

	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			CD3D11_RENDER_TARGET_VIEW_DESC descRTV(D3D11_RTV_DIMENSION_TEXTURE2DARRAY, descTex.Format, i, j, 1);
			g_pDevice->CreateRenderTargetView(m_pTexPreFilterMap, &descRTV, &m_pRTVPreFilterMaps[i][j]);
		}
	}

	std::vector<VertexP> vertices =
	{
		// +X
		{ Vector3(+0.5f, +0.5f, -0.5f) },
		{ Vector3(+0.5f, +0.5f, +0.5f) },
		{ Vector3(+0.5f, -0.5f, +0.5f) },
		{ Vector3(+0.5f, -0.5f, -0.5f) },

		// -X
		{ Vector3(-0.5f, +0.5f, +0.5f) },
		{ Vector3(-0.5f, +0.5f, -0.5f) },
		{ Vector3(-0.5f, -0.5f, -0.5f) },
		{ Vector3(-0.5f, -0.5f, +0.5f) },

		// +Y
		{ Vector3(-0.5f, +0.5f, +0.5f) },
		{ Vector3(+0.5f, +0.5f, +0.5f) },
		{ Vector3(+0.5f, +0.5f, -0.5f) },
		{ Vector3(-0.5f, +0.5f, -0.5f) },

		// -Y
		{ Vector3(-0.5f, -0.5f, -0.5f) },
		{ Vector3(+0.5f, -0.5f, -0.5f) },
		{ Vector3(+0.5f, -0.5f, +0.5f) },
		{ Vector3(-0.5f, -0.5f, +0.5f) },

		// +Z
		{ Vector3(+0.5f, +0.5f, +0.5f) },
		{ Vector3(-0.5f, +0.5f, +0.5f) },
		{ Vector3(-0.5f, -0.5f, +0.5f) },
		{ Vector3(+0.5f, -0.5f, +0.5f) },

		// -Z
		{ Vector3(-0.5f, +0.5f, -0.5f) },
		{ Vector3(+0.5f, +0.5f, -0.5f) },
		{ Vector3(+0.5f, -0.5f, -0.5f) },
		{ Vector3(-0.5f, -0.5f, -0.5f) },
	};

	std::vector<USHORT> indices =
	{
		0,  2,	1,
		0,  3,	2,

		4,  6,	5,
		4,  7,	6,

		8,  10,	9,
		8,  11,	10,

		12, 14,	13,
		12, 15,	14,

		16, 18,	17,
		16, 19,	18,

		20, 22,	21,
		20, 23,	22,
	};

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(VertexP) * (UINT)vertices.size();
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = vertices.data();

	ID3D11Buffer* pVertexBuffer;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &pVertexBuffer));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(USHORT) * (UINT)indices.size();
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	InitData.pSysMem = indices.data();

	ID3D11Buffer* pIndexBuffer;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &pIndexBuffer));

	ID3D11InputLayout* pInputLayoutPNT;
	ID3D11VertexShader* pVertexShader;
	ID3D11PixelShader* pPixelShader;
	ID3DBlob* pVSBlob = nullptr;
	ID3DBlob* pPSBlob = nullptr;

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\CubeMapPreFilter.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &pVertexShader));

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPNT, ARRAYSIZE(NXGlobalInputLayout::layoutPNT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &pInputLayoutPNT));
	g_pContext->IASetInputLayout(pInputLayoutPNT);

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\CubeMapPreFilter.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pPixelShader));

	pVSBlob->Release();
	pPSBlob->Release();

	ID3D11Buffer* cbCubeCamera = nullptr;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferObject);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &cbCubeCamera));

	ID3D11Buffer* cbRoughness = nullptr;
	bufferDesc.ByteWidth = sizeof(ConstantBufferObject);
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &cbRoughness));

	Matrix mxCubeMapProj = XMMatrixPerspectiveFovLH(XMConvertToRadians(90.0f), 1.0f, 0.1f, 10.0f);
	Matrix mxCubeMapView[6] =
	{
		XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 1.0f,  0.0f,  0.0f), Vector3(0.0f, 1.0f,  0.0f)),
		XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3(-1.0f,  0.0f,  0.0f), Vector3(0.0f, 1.0f,  0.0f)),
		XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 0.0f,  1.0f,  0.0f), Vector3(0.0f, 0.0f, -1.0f)),
		XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 0.0f, -1.0f,  0.0f), Vector3(0.0f, 0.0f,  1.0f)),
		XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 0.0f,  0.0f,  1.0f), Vector3(0.0f, 1.0f,  0.0f)),
		XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 0.0f,  0.0f, -1.0f), Vector3(0.0f, 1.0f,  0.0f))
	};

	g_pContext->VSSetShader(pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(pPixelShader, nullptr, 0);
	g_pContext->PSSetShaderResources(0, 1, &m_pSRVCubeMap);
	g_pContext->VSSetConstantBuffers(0, 1, &cbCubeCamera);
	g_pContext->PSSetConstantBuffers(1, 1, &cbRoughness);

	ConstantBufferObject cbDataCubeCamera;
	cbDataCubeCamera.world = Matrix::Identity();
	cbDataCubeCamera.projection = mxCubeMapProj.Transpose();

	UINT stride = sizeof(VertexP);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	float roughValues[5] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
	ConstantBufferFloat cbDataRoughness;
	cbDataRoughness.Value = 0;

	UINT uMapSize = (UINT)MapSize;
	CD3D11_VIEWPORT vp[5];
	for (int i = 0; i < 5; i++)
	{
		vp[i] = CD3D11_VIEWPORT(0.0f, 0.0f, (float)(uMapSize >> i), (float)(uMapSize >> i));
	}

	for (int i = 0; i < 5; i++)
	{
		cbDataRoughness.Value = roughValues[i];
		g_pContext->RSSetViewports(1, &vp[i]);
		for (int j = 0; j < 6; j++)
		{
			cbDataCubeCamera.view = mxCubeMapView[j].Transpose();
			g_pContext->ClearRenderTargetView(m_pRTVPreFilterMaps[i][j], Colors::WhiteSmoke);
			g_pContext->OMSetRenderTargets(1, &m_pRTVPreFilterMaps[i][j], nullptr);

			g_pContext->UpdateSubresource(cbCubeCamera, 0, nullptr, &cbDataCubeCamera, 0, 0);
			g_pContext->UpdateSubresource(cbRoughness, 0, nullptr, &cbDataRoughness, 0, 0);
			g_pContext->DrawIndexed((UINT)indices.size() / 6, j * 6, 0);
		}
	}

	pVertexBuffer->Release();
	pIndexBuffer->Release();
	pVertexShader->Release();
	pPixelShader->Release();
	pInputLayoutPNT->Release();
	cbCubeCamera->Release();
	cbRoughness->Release();
}

void NXCubeMap::GenerateBRDF2DLUT()
{
	const static float MapSize = 512.0f;
	CD3D11_VIEWPORT vp(0.0f, 0.0f, MapSize, MapSize);
	g_pContext->RSSetViewports(1, &vp);

	CD3D11_TEXTURE2D_DESC descTex(DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)MapSize, (UINT)MapSize, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, 1, 0, 0);
	g_pDevice->CreateTexture2D(&descTex, nullptr, &m_pTexBRDF2DLUT);

	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(D3D11_SRV_DIMENSION_TEXTURE2D, descTex.Format, 0, descTex.MipLevels, 0, descTex.ArraySize);
	g_pDevice->CreateShaderResourceView(m_pTexBRDF2DLUT, &descSRV, &m_pSRVBRDF2DLUT); 

	CD3D11_RENDER_TARGET_VIEW_DESC descRTV(D3D11_RTV_DIMENSION_TEXTURE2D, descTex.Format);
	g_pDevice->CreateRenderTargetView(m_pTexBRDF2DLUT, &descRTV, &m_pRTVBRDF2DLUT);

	std::vector<VertexPT> vertices =
	{
		// +Z
		{ Vector3(+1.0f, +1.0f, +1.0f), Vector2(1.0f, 1.0f) },
		{ Vector3(-1.0f, +1.0f, +1.0f), Vector2(1.0f, 0.0f) },
		{ Vector3(-1.0f, -1.0f, +1.0f), Vector2(0.0f, 0.0f) },
		{ Vector3(+1.0f, -1.0f, +1.0f), Vector2(0.0f, 1.0f) },
	};
	 
	std::vector<USHORT> indices =
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

	ID3D11Buffer* pVertexBuffer;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &pVertexBuffer));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(USHORT) * (UINT)indices.size();
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	InitData.pSysMem = indices.data();

	ID3D11Buffer* pIndexBuffer;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &pIndexBuffer));

	ID3D11InputLayout* pInputLayoutPT;
	ID3D11VertexShader* pVertexShader;
	ID3D11PixelShader* pPixelShader;
	ID3DBlob* pVSBlob = nullptr;
	ID3DBlob* pPSBlob = nullptr;

	NX::MessageBoxIfFailed( 
		ShaderComplier::Compile(L"Shader\\BRDF2DLUT.fx", "VS", "vs_5_0", &pVSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &pVertexShader));

	NX::ThrowIfFailed(g_pDevice->CreateInputLayout(NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &pInputLayoutPT));
	g_pContext->IASetInputLayout(pInputLayoutPT);

	NX::MessageBoxIfFailed(
		ShaderComplier::Compile(L"Shader\\BRDF2DLUT.fx", "PS", "ps_5_0", &pPSBlob),
		L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.");
	NX::ThrowIfFailed(g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &pPixelShader));

	pVSBlob->Release();
	pPSBlob->Release();

	Matrix mxCubeMapProj = XMMatrixPerspectiveFovLH(XMConvertToRadians(90.0f), 1.0f, 0.1f, 10.0f);
	Matrix mxCubeMapView = XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f));

	g_pContext->VSSetShader(pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(pPixelShader, nullptr, 0);

	UINT stride = sizeof(VertexPT); 
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
	g_pContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	{
		g_pContext->ClearRenderTargetView(m_pRTVBRDF2DLUT, Colors::WhiteSmoke);
		g_pContext->OMSetRenderTargets(1, &m_pRTVBRDF2DLUT, nullptr);
		g_pContext->DrawIndexed((UINT)indices.size(), 0, 0);
	}

	pVertexBuffer->Release();
	pIndexBuffer->Release();
	pVertexShader->Release();
	pPixelShader->Release();
	pInputLayoutPT->Release();
}

void NXCubeMap::InitVertex()
{
	int currVertIdx = 0;
	int segmentVertical = 32;
	int segmentHorizontal = 32;
	for (int i = 0; i < segmentVertical; i++)
	{
		float yDown = sinf(((float)i / (float)segmentVertical * 2.0f - 1.0f) * XM_PIDIV2);
		float yUp = sinf(((float)(i + 1) / (float)segmentVertical * 2.0f - 1.0f) * XM_PIDIV2);
		float radiusDown = sqrtf(1.0f - yDown * yDown);
		float radiusUp = sqrtf(1.0f - yUp * yUp);

		float yUVUp = Clamp(yUp * 0.5f + 0.5f, 0.0f, 1.0f);
		float yUVDown = Clamp(yDown * 0.5f + 0.5f, 0.0f, 1.0f);

		for (int j = 0; j < segmentHorizontal; j++)
		{
			float segNow = (float)j / (float)segmentHorizontal;
			float segNext = (float)(j + 1) / (float)segmentHorizontal;
			float angleNow = segNow * XM_2PI;
			float angleNext = segNext * XM_2PI;
			float xNow = cosf(angleNow);
			float zNow = sinf(angleNow);
			float xNext = cosf(angleNext);
			float zNext = sinf(angleNext);

			Vector3 pNowUp = { xNow * radiusUp, yUp, zNow * radiusUp };
			Vector3 pNextUp = { xNext * radiusUp, yUp, zNext * radiusUp };
			Vector3 pNowDown = { xNow * radiusDown, yDown, zNow * radiusDown };
			Vector3 pNextDown = { xNext * radiusDown, yDown, zNext * radiusDown };

			m_vertices.push_back(pNowUp);
			m_vertices.push_back(pNextUp);
			m_vertices.push_back(pNextDown);
			m_vertices.push_back(pNowDown);

			m_indices.push_back(currVertIdx);
			m_indices.push_back(currVertIdx + 2);
			m_indices.push_back(currVertIdx + 1);
			m_indices.push_back(currVertIdx);
			m_indices.push_back(currVertIdx + 3);
			m_indices.push_back(currVertIdx + 2);

			currVertIdx += 4;
		}
	}

	InitVertexIndexBuffer();
}

void NXCubeMap::InitVertexIndexBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(VertexP) * (UINT)m_vertices.size();
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = m_vertices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pVertexBuffer));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(USHORT) * (UINT)m_indices.size();
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	InitData.pSysMem = m_indices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pIndexBuffer));
}
