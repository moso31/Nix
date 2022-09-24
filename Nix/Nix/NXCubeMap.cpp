#include "NXCubeMap.h"
#include "SphereHarmonics.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXCamera.h"
#include "ShaderComplier.h"
#include "DirectResources.h"
#include "NXRenderStates.h"


using namespace DirectX::SimpleMath::SH;

NXCubeMap::NXCubeMap(NXScene* pScene) :
	m_pScene(pScene),
	m_format(DXGI_FORMAT_UNKNOWN),
	m_height(0),
	m_width(0)
{
	InitConstantBuffer();
}

bool NXCubeMap::Init(const std::wstring filePath)
{
	// create vertex
	InitVertex();

	m_mxCubeMapProj = XMMatrixPerspectiveFovLH(XMConvertToRadians(90.0f), 1.0f, 0.1f, 10.0f);
	m_mxCubeMapView[0] = XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 1.0f,  0.0f,  0.0f), Vector3(0.0f,  1.0f,  0.0f));
	m_mxCubeMapView[1] = XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3(-1.0f,  0.0f,  0.0f), Vector3(0.0f,  1.0f,  0.0f));
	m_mxCubeMapView[2] = XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 0.0f,  1.0f,  0.0f), Vector3(0.0f,  0.0f, -1.0f));
	m_mxCubeMapView[3] = XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 0.0f, -1.0f,  0.0f), Vector3(0.0f,  0.0f,  1.0f));
	m_mxCubeMapView[4] = XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 0.0f,  0.0f,  1.0f), Vector3(0.0f,  1.0f,  0.0f));
	m_mxCubeMapView[5] = XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3( 0.0f,  0.0f, -1.0f), Vector3(0.0f,  1.0f,  0.0f));

	m_pImage.reset(); 
	m_pImage = std::make_unique<ScratchImage>();

	TexMetadata HDRInfo;
	HRESULT hr; 
	std::wstring suffix = filePath.substr(filePath.rfind(L"."));

	// 创建CubeMap SRV。
	// 需要使用初始格式创建，也就是说要在Decompress之前创建。
	if (_wcsicmp(suffix.c_str(), L".dds") == 0)
	{
		m_cubeMapFilePath = filePath;
		hr = LoadFromDDSFile(filePath.c_str(), DDS_FLAGS_NONE, &HDRInfo, *m_pImage);
		m_format = HDRInfo.format;

		std::unique_ptr<ScratchImage> pImageMip = std::make_unique<ScratchImage>();
		hr = GenerateMipMaps(m_pImage->GetImages(), m_pImage->GetImageCount(), m_pImage->GetMetadata(), TEX_FILTER_DEFAULT, 0, *pImageMip);
		HDRInfo.mipLevels = pImageMip->GetMetadata().mipLevels;
		m_pImage.swap(pImageMip);

		hr = CreateTextureEx(g_pDevice.Get(), m_pImage->GetImages(), m_pImage->GetImageCount(), HDRInfo, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, false, (ID3D11Resource**)m_pTexCubeMap.GetAddressOf());
		hr = CreateShaderResourceView(g_pDevice.Get(), m_pImage->GetImages(), m_pImage->GetImageCount(), HDRInfo, &m_pSRVCubeMap);
	}
	else if (_wcsicmp(suffix.c_str(), L".hdr") == 0)
	{
		auto pHDRImage = std::make_unique<ScratchImage>();
		hr = LoadFromHDRFile(filePath.c_str(), &HDRInfo, *pHDRImage);
		m_format = HDRInfo.format;

		hr = CreateShaderResourceViewEx(g_pDevice.Get(), pHDRImage->GetImages(), pHDRImage->GetImageCount(), HDRInfo, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, (UINT)HDRInfo.miscFlags, false, &m_pSRVHDRMap);
		
		GenerateCubeMap(filePath);
		hr = LoadFromDDSFile(m_cubeMapFilePath.c_str(), DDS_FLAGS_NONE, &HDRInfo, *m_pImage);

		size_t imgWidth = pHDRImage->GetMetadata().width;
		size_t imgHeight = pHDRImage->GetMetadata().height;
		GenerateIrradianceSH(imgWidth, imgHeight);

		//// HDRI 纹理加载
		//auto pData = reinterpret_cast<float*>(pHDRImage->GetImage(0, 0, 0)->pixels);
		//double solidAnglePdf = 0.0;
		//double test = 0.0;

		//memset(m_shIrradianceMap, 0, sizeof(m_shIrradianceMap));

		//// 像素个数
		//size_t pixelCount = pHDRImage->GetPixelsSize() >> 4;

		//size_t threadCount = imgHeight;
		//for (int threadIdx = 0; threadIdx < (int)threadCount; threadIdx++)
		//{
		//	int threadSize = pixelCount / threadCount;
		//	for (int i = 0; i < threadSize; i++)
		//	{
		//		size_t idx = threadIdx * threadSize + i;

		//		double u = (double(idx % imgWidth) + 0.5) / imgWidth;
		//		double v = (double(idx / imgWidth) + 0.5) / imgHeight;

		//		double scaleY = 0.5 / imgHeight;
		//		double thetaU = (v - scaleY) * XM_PI;
		//		double thetaD = (v + scaleY) * XM_PI;

		//		double dPhi = XM_2PI / imgWidth;	// dPhi 是个常量
		//		double dTheta = cos(thetaU) - cos(thetaD);
		//		solidAnglePdf = dPhi * dTheta;

		//		// get L(Rs).
		//		size_t offset = idx << 2;
		//		Vector3 incidentRadiance(pData + offset);

		//		for (int l = 0; l < 3; l++)
		//		{
		//			for (int m = -l; m <= l; m++)
		//			{
		//				float sh = SHBasis(l, m, v * XM_PI, XM_3PIDIV2 - u * XM_2PI);  // HDRI纹理角度矫正

		//				// sh = y_l^m(Rs)
		//				// m_shIrradianceMap[k++] = L_l^m
		//				Vector3 Llm = incidentRadiance * sh * solidAnglePdf;
		//				printf("%d | %d, %d | %f, %f, %f\n", idx, l, m, Llm.x, Llm.y, Llm.z);
		//				{
		//					m_shIrradianceMap[l * l + l + m] += Llm;
		//				}
		//			}
		//		}

		//		printf("\n");
		//	}
		//}

		//const float T[5] = { 0.886226925452757f, 1.023326707946489f, 0.495415912200751f, 0.0f, -0.110778365681594f };
		//int k = 0;
		//for (int l = 0; l < 3; l++)
		//{
		//	for (int m = -l; m <= l; m++)
		//	{
		//		// 求 E_l^m
		//		m_shIrradianceMap[k++] *= sqrt(XM_4PI / (2.0f * l + 1.0f)) * T[l] * XM_1DIVPI;
		//	}
		//}

		std::unique_ptr<ScratchImage> pImageMip = std::make_unique<ScratchImage>();
		hr = GenerateMipMaps(m_pImage->GetImages(), m_pImage->GetImageCount(), m_pImage->GetMetadata(), TEX_FILTER_DEFAULT, 0, *pImageMip);
		HDRInfo.mipLevels = pImageMip->GetMetadata().mipLevels;
		m_pImage.swap(pImageMip); 

		hr = CreateTextureEx(g_pDevice.Get(), m_pImage->GetImages(), m_pImage->GetImageCount(), HDRInfo, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, false, (ID3D11Resource**)m_pTexCubeMap.GetAddressOf());
		hr = CreateShaderResourceView(g_pDevice.Get(), m_pImage->GetImages(), m_pImage->GetImageCount(), HDRInfo, &m_pSRVCubeMap);

		auto HDRPreviewInfo = HDRInfo;
		HDRPreviewInfo.arraySize = 1;
		HDRPreviewInfo.mipLevels = 1;
		HDRPreviewInfo.miscFlags = 0;
		hr = CreateShaderResourceView(g_pDevice.Get(), m_pImage->GetImage(0, 0, 0), 1, HDRPreviewInfo, &m_pSRVCubeMapPreview2D);
	}

	if (IsCompressed(HDRInfo.format))
	{
		auto img = m_pImage->GetImage(0, 0, 0);
		size_t nimg = m_pImage->GetImageCount();

		std::unique_ptr<ScratchImage> dcImage = std::make_unique<ScratchImage>();
		hr = Decompress(img, nimg, HDRInfo, DXGI_FORMAT_UNKNOWN /* picks good default */, *dcImage);
		if (SUCCEEDED(hr))
		{
			if (dcImage && dcImage->GetPixels())
				m_pImage.swap(dcImage);
		}
	}

	if (!m_pImage) return false;

	m_width = HDRInfo.width;
	m_height = HDRInfo.height;

	m_faceData.resize(6);
	for (int item = 0; item < 6; item++)
	{
		auto faceImage = m_pImage->GetImage(0, item, 0);
		m_faceData[item] = faceImage->pixels;
	}

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
	// cubemap params
	g_pContext->UpdateSubresource(m_cb.Get(), 0, nullptr, &m_cbData, 0, 0);
}

void NXCubeMap::UpdateViewParams()
{
	auto pCamera = m_pScene->GetMainCamera();
	NXGlobalBufferManager::m_cbDataObject.world = Matrix::CreateTranslation(pCamera->GetTranslation()).Transpose();
	g_pContext->UpdateSubresource(NXGlobalBufferManager::m_cbObject.Get(), 0, nullptr, &NXGlobalBufferManager::m_cbDataObject, 0, 0);
}

void NXCubeMap::Render()
{
	UINT stride = sizeof(VertexP);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);
}

void NXCubeMap::Release()
{
	NXTransform::Release();
}

void NXCubeMap::GenerateCubeMap(const std::wstring filePath)
{
	g_pUDA->BeginEvent(L"Generate Cube Map");

	ComPtr<ID3D11SamplerState> pSamplerState;
	pSamplerState.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::Create());
	g_pContext->PSSetSamplers(0, 1, pSamplerState.GetAddressOf());

	ComPtr<ID3D11Texture2D> pTexCubeMap;
	ComPtr<ID3D11ShaderResourceView> pSRVCubeMap;
	ComPtr<ID3D11RenderTargetView> pRTVCubeMaps[6];

	const static float MapSize = 1024;
	CD3D11_VIEWPORT vp(0.0f, 0.0f, MapSize, MapSize);
	g_pContext->RSSetViewports(1, &vp);

	CD3D11_TEXTURE2D_DESC descTex(DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)MapSize, (UINT)MapSize, 6, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, D3D11_CPU_ACCESS_READ, 1, 0, D3D11_RESOURCE_MISC_TEXTURECUBE);
	g_pDevice->CreateTexture2D(&descTex, nullptr, &pTexCubeMap);

	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(D3D11_SRV_DIMENSION_TEXTURECUBE, descTex.Format, 0, descTex.MipLevels, 0, descTex.ArraySize);
	g_pDevice->CreateShaderResourceView(pTexCubeMap.Get(), &descSRV, &pSRVCubeMap);

	for (int i = 0; i < 6; i++)
	{
		CD3D11_RENDER_TARGET_VIEW_DESC descRTV(D3D11_RTV_DIMENSION_TEXTURE2DARRAY, descTex.Format, 0, i, 1);
		g_pDevice->CreateRenderTargetView(pTexCubeMap.Get(), &descRTV, &pRTVCubeMaps[i]);
	}

	ComPtr<ID3D11InputLayout> pInputLayoutP;
	ComPtr<ID3D11VertexShader> pVertexShader;
	ComPtr<ID3D11PixelShader> pPixelShader;

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\HDRToCubeMap.fx", "VS", &pVertexShader, NXGlobalInputLayout::layoutP, ARRAYSIZE(NXGlobalInputLayout::layoutP), &pInputLayoutP);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\HDRToCubeMap.fx", "PS", &pPixelShader);
	g_pContext->VSSetShader(pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(pInputLayoutP.Get());

	D3D11_BUFFER_DESC bufferDesc;
	ComPtr<ID3D11Buffer> cb;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferObject);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &cb));

	g_pContext->PSSetShaderResources(0, 1, m_pSRVHDRMap.GetAddressOf());
	g_pContext->VSSetConstantBuffers(0, 1, cb.GetAddressOf());

	ConstantBufferObject cbData;
	cbData.world = Matrix::Identity();
	cbData.projection = m_mxCubeMapProj.Transpose();

	UINT stride = sizeof(VertexP);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, m_pVertexBufferCubeBox.GetAddressOf(), &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBufferCubeBox.Get(), DXGI_FORMAT_R32_UINT, 0);

	for (int i = 0; i < 6; i++)
	{
		g_pContext->ClearRenderTargetView(pRTVCubeMaps[i].Get(), Colors::WhiteSmoke);
		g_pContext->OMSetRenderTargets(1, pRTVCubeMaps[i].GetAddressOf(), nullptr);

		cbData.view = m_mxCubeMapView[i].Transpose();
		g_pContext->UpdateSubresource(cb.Get(), 0, nullptr, &cbData, 0, 0);
		g_pContext->DrawIndexed((UINT)m_indicesCubeBox.size() / 6, i * 6, 0);
	}

	ComPtr<ID3D11Texture2D> pMappedTexture;
	D3D11_MAPPED_SUBRESOURCE pMappedTextureInfo;
	HRESULT hr = g_pContext->Map(pTexCubeMap.Get(), 0, D3D11_MAP_READ, 0, &pMappedTextureInfo);

	// 如果Map失败并且hr标记为E_INVALIDARG，说明此时pTexCubeMap正在被占用中。
	// 此时可创建一个STAGING资源，将GPU中的纹理资源（pTexCubeMap）从显存经设备上下文（g_pContext）映射（Map）到临时的STAGING纹理（pTexStaging），再进行CopyResource。
	// 这样即使原资源（pTexCubeMap）被渲染管线占用，也能拿到对应的值。
	// 该思路严格来说算是异步操作。参考自：https://www.zhihu.com/question/35068373
	if (hr == E_INVALIDARG)
	{
		D3D11_TEXTURE2D_DESC descMappedTex;
		descMappedTex.Width = descTex.Width;
		descMappedTex.Height = descTex.Height;
		descMappedTex.MipLevels = descTex.MipLevels;
		descMappedTex.ArraySize = descTex.ArraySize;
		descMappedTex.Format = descTex.Format;
		descMappedTex.SampleDesc = descTex.SampleDesc;
		descMappedTex.Usage = D3D11_USAGE_STAGING; // 仅CPU可访问。
		descMappedTex.BindFlags = 0;
		descMappedTex.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		descMappedTex.MiscFlags = 0;

		// 创建pTexStaging纹理，仅用于存储中间数据。
		ComPtr<ID3D11Texture2D> pTexStaging;
		hr = g_pDevice->CreateTexture2D(&descMappedTex, nullptr, &pTexStaging);

		// 将DrawIndex绘制好的资源复制到pTexStaging。
		g_pContext->CopyResource(pTexStaging.Get(), pTexCubeMap.Get());

		// 映射到pMappedTextureInfo
		hr = g_pContext->Map(pTexStaging.Get(), 0, D3D11_MAP_READ, 0, &pMappedTextureInfo);

		if (FAILED(hr))
		{
			throw(hr);
		}

		// copyRecourse后，将pTexStaging强制move给pMappedTexture。
		// pMappedTexture鸠占鹊巢成为实际上管理纹理数据的指针。pTexStaging则被置nullptr，不再具备任何实际功能。
		pMappedTexture = std::move(pTexStaging);
	}
	else
	{
		// 如果直接成功Map说明资源没有被占用。
		// 那就没上面那么多事了……直接拿就行了。
		pMappedTexture = pTexCubeMap;
	}

	g_pContext->Unmap(pMappedTexture.Get(), 0);
	g_pUDA->EndEvent();

	std::unique_ptr<ScratchImage> pMappedImage = std::make_unique<ScratchImage>();
	hr = CaptureTexture(g_pDevice.Get(), g_pContext.Get(), pMappedTexture.Get(), *pMappedImage);
	TexMetadata cubeDDSInfo = pMappedImage->GetMetadata();
	cubeDDSInfo.miscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

	size_t pathLen = filePath.length();
	size_t fileIndex = filePath.rfind(L"\\") + 1;
	std::wstring fileNameAndSuffix = filePath.substr(fileIndex, pathLen - fileIndex);
	size_t suffixIndex = fileNameAndSuffix.find(L".");
	std::wstring fileName = fileNameAndSuffix.substr(0, suffixIndex);
	m_cubeMapFilePath = L"D:\\" + fileName + L".dds";
	hr = SaveToDDSFile(pMappedImage->GetImages(), pMappedImage->GetImageCount(), cubeDDSInfo, DDS_FLAGS_NONE, m_cubeMapFilePath.c_str());
}

void NXCubeMap::GenerateIrradianceSH(size_t imgWidth, size_t imgHeight)
{
	g_pUDA->BeginEvent(L"Generate Irradiance Map SH");

	ComPtr<ID3D11SamplerState> pSamplerState;
	pSamplerState.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP>::Create());
	g_pContext->CSSetSamplers(0, 1, pSamplerState.GetAddressOf());

	ComPtr<ID3D11Buffer> cbImageSize;
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferImageData);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &cbImageSize));

	UINT tempWidth = (UINT)imgWidth;
	UINT tempHeight = (UINT)imgHeight;
	UINT SHIrradPassCount = 0;
	while (tempWidth != 1 || tempHeight != 1)
	{
		tempWidth = max((tempWidth + 7) / 8, 1);
		tempHeight = max((tempHeight + 7) / 8, 1);
		SHIrradPassCount++;
	}
	SHIrradPassCount = max(SHIrradPassCount, 2);

	tempWidth = (UINT)imgWidth;
	tempHeight = (UINT)imgHeight;
	std::vector<ComPtr<ID3D11ShaderResourceView>> pSRVIrradSHs;
	pSRVIrradSHs.reserve(SHIrradPassCount);
	for (UINT passId = 0; passId < SHIrradPassCount; passId++)
	{
		std::wstring eventName = L"SH Irradiance Gather" + std::to_wstring(passId);
		g_pUDA->BeginEvent(eventName.c_str());

		UINT currWidth = tempWidth;
		UINT currHeight = tempHeight;

		tempWidth = max((tempWidth + 7) / 8, 1);
		tempHeight = max((tempHeight + 7) / 8, 1);

		ConstantBufferImageData cbImageSizeData;
		cbImageSizeData.currImgSize = Vector4((float)currWidth, (float)currHeight, 1.0f / (float)currWidth, 1.0f / (float)currHeight);
		cbImageSizeData.nextImgSize = Vector4((float)tempWidth, (float)tempHeight, 1.0f / (float)tempWidth, 1.0f / (float)tempHeight);

		g_pContext->UpdateSubresource(cbImageSize.Get(), 0, nullptr, &cbImageSizeData, 0, 0);
		g_pContext->CSSetConstantBuffers(0, 1, cbImageSize.GetAddressOf());

		size_t irradianceBufferElements = tempWidth * tempHeight;
		size_t irradianceBufferSize = sizeof(ConstantBufferIrradSH) * irradianceBufferElements;

		D3D11_BUFFER_DESC bufferDesc;
		ComPtr<ID3D11Buffer> cbIrradianceSH;
		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.ByteWidth = (UINT)irradianceBufferSize;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.StructureByteStride = sizeof(ConstantBufferIrradSH);
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &cbIrradianceSH));

		ComPtr<ID3D11ComputeShader> pComputeShader;
		std::wstring strCSPath = L"";

		// 设置 CubeMapIrradianceSH.fx 使用哪个入口点函数
		if (passId == 0)
		{
			CD3D_SHADER_MACRO macro("CUBEMAP_IRRADSH_FIRST", "1");
			NXShaderComplier::GetInstance()->AddMacro(macro);
		}
		else if (passId > 0 && passId < SHIrradPassCount - 1)
		{
			CD3D_SHADER_MACRO macro("CUBEMAP_IRRADSH_MIDDLE", "1");
			NXShaderComplier::GetInstance()->AddMacro(macro);
		}
		else
		{
			CD3D_SHADER_MACRO macro("CUBEMAP_IRRADSH_LAST", "1");
			NXShaderComplier::GetInstance()->AddMacro(macro);
		}

		NXShaderComplier::GetInstance()->CompileCS(L"Shader\\CubeMapIrradianceSH.fx", "CS", &pComputeShader);

		ComPtr<ID3D11UnorderedAccessView> pUAVIrradSH;
		CD3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc(D3D11_UAV_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, 0, (UINT)irradianceBufferElements);
		NX::ThrowIfFailed(g_pDevice->CreateUnorderedAccessView(cbIrradianceSH.Get(), &UAVDesc, pUAVIrradSH.GetAddressOf()));

		ComPtr<ID3D11ShaderResourceView> pSRVIrradSH;
		CD3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc(D3D11_SRV_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, 0, (UINT)irradianceBufferElements);
		NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(cbIrradianceSH.Get(), &SRVDesc, pSRVIrradSH.GetAddressOf()));

		std::string UAVDebugName = "SHIrrad Buffer UAV" + std::to_string(passId);
		pUAVIrradSH->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)UAVDebugName.size(), UAVDebugName.c_str());

		std::string SRVDebugName = "SHIrrad Buffer SRV" + std::to_string(passId);
		pSRVIrradSH->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)SRVDebugName.size(), SRVDebugName.c_str());
		pSRVIrradSHs.push_back(pSRVIrradSH);
		if (passId == SHIrradPassCount - 1) 
			m_pSRVIrradianceSH = pSRVIrradSH;

		g_pContext->CSSetUnorderedAccessViews(0, 1, pUAVIrradSH.GetAddressOf(), nullptr);

		if (passId == 0)
		{
			g_pContext->CSSetShaderResources(0, 1, m_pSRVHDRMap.GetAddressOf());
		}
		else
		{
			g_pContext->CSSetShaderResources(0, 1, pSRVIrradSHs[passId - 1].GetAddressOf());
		}

		g_pContext->CSSetShader(pComputeShader.Get(), nullptr, 0);

		g_pContext->Dispatch(tempWidth, tempHeight, 1);

		// 用完以后清空对应槽位的SRV UAV，避免后续pass资源绑不上（可能有优化空间）
		ComPtr<ID3D11ShaderResourceView> pSRVNull[1] = { nullptr };
		g_pContext->CSSetShaderResources(0, 1, pSRVNull->GetAddressOf());

		ComPtr<ID3D11UnorderedAccessView> pUAVNull[1] = { nullptr };
		g_pContext->CSSetUnorderedAccessViews(0, 1, pUAVNull->GetAddressOf(), nullptr);

		g_pUDA->EndEvent();
	}

	g_pUDA->EndEvent();
}

void NXCubeMap::GenerateIrradianceMap()
{
	g_pUDA->BeginEvent(L"Generate Irradiance Map");

	ComPtr<ID3D11SamplerState> pSamplerState;
	pSamplerState.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::Create());
	g_pContext->PSSetSamplers(0, 1, pSamplerState.GetAddressOf());

	const static float MapSize = 32.0f;
	CD3D11_VIEWPORT vp(0.0f, 0.0f, MapSize, MapSize);
	g_pContext->RSSetViewports(1, &vp);

	CD3D11_TEXTURE2D_DESC descTex(m_format, (UINT)MapSize, (UINT)MapSize, 6, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, 1, 0, D3D11_RESOURCE_MISC_TEXTURECUBE);
	g_pDevice->CreateTexture2D(&descTex, nullptr, &m_pTexIrradianceMap);

	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(D3D11_SRV_DIMENSION_TEXTURECUBE, descTex.Format, 0, descTex.MipLevels, 0, descTex.ArraySize);
	g_pDevice->CreateShaderResourceView(m_pTexIrradianceMap.Get(), &descSRV, &m_pSRVIrradianceMap);

	for (int i = 0; i < 6; i++)
	{
		CD3D11_RENDER_TARGET_VIEW_DESC descRTV(D3D11_RTV_DIMENSION_TEXTURE2DARRAY, descTex.Format, 0, i, 1);
		g_pDevice->CreateRenderTargetView(m_pTexIrradianceMap.Get(), &descRTV, &m_pRTVIrradianceMaps[i]);
	}

	ComPtr<ID3D11InputLayout> pInputLayoutP;
	ComPtr<ID3D11VertexShader> pVertexShader;
	ComPtr<ID3D11PixelShader> pPixelShader;

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\CubeMapIrradiance.fx", "VS", &pVertexShader, NXGlobalInputLayout::layoutPNT, ARRAYSIZE(NXGlobalInputLayout::layoutPNT), &pInputLayoutP);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\CubeMapIrradiance.fx", "PS", &pPixelShader);
	g_pContext->VSSetShader(pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(pInputLayoutP.Get());

	ComPtr<ID3D11Buffer> cb;
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferObject);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &cb));

	g_pContext->PSSetShaderResources(0, 1, m_pSRVCubeMap.GetAddressOf());
	g_pContext->VSSetConstantBuffers(0, 1, cb.GetAddressOf());

	ConstantBufferObject cbData;
	cbData.world = Matrix::Identity();
	cbData.projection = m_mxCubeMapProj.Transpose();

	UINT stride = sizeof(VertexP);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, m_pVertexBufferCubeBox.GetAddressOf(), &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBufferCubeBox.Get(), DXGI_FORMAT_R32_UINT, 0);

	for (int i = 0; i < 6; i++)
	{
		g_pContext->ClearRenderTargetView(m_pRTVIrradianceMaps[i].Get(), Colors::WhiteSmoke);
		g_pContext->OMSetRenderTargets(1, m_pRTVIrradianceMaps[i].GetAddressOf(), nullptr);

		cbData.view = m_mxCubeMapView[i].Transpose();
		g_pContext->UpdateSubresource(cb.Get(), 0, nullptr, &cbData, 0, 0);
		g_pContext->DrawIndexed((UINT)m_indicesCubeBox.size() / 6, i * 6, 0);
	}

	g_pUDA->EndEvent();
}

void NXCubeMap::GeneratePreFilterMap()
{
	g_pUDA->BeginEvent(L"Generate PreFilter Map");

	ComPtr<ID3D11SamplerState> pSamplerState;
	pSamplerState.Swap(NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::Create());
	g_pContext->PSSetSamplers(0, 1, pSamplerState.GetAddressOf());

	const static float MapSize = 512.0f;
	CD3D11_TEXTURE2D_DESC descTex(m_format, (UINT)MapSize, (UINT)MapSize, 6, 5, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, 1, 0, D3D11_RESOURCE_MISC_TEXTURECUBE);
	g_pDevice->CreateTexture2D(&descTex, nullptr, &m_pTexPreFilterMap);

	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(D3D11_SRV_DIMENSION_TEXTURECUBE, descTex.Format, 0, descTex.MipLevels, 0, descTex.ArraySize);
	g_pDevice->CreateShaderResourceView(m_pTexPreFilterMap.Get(), &descSRV, &m_pSRVPreFilterMap);

	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			CD3D11_RENDER_TARGET_VIEW_DESC descRTV(D3D11_RTV_DIMENSION_TEXTURE2DARRAY, descTex.Format, i, j, 1);
			g_pDevice->CreateRenderTargetView(m_pTexPreFilterMap.Get(), &descRTV, &m_pRTVPreFilterMaps[i][j]);
		}
	}

	ComPtr<ID3D11InputLayout> pInputLayoutP;
	ComPtr<ID3D11VertexShader> pVertexShader;
	ComPtr<ID3D11PixelShader> pPixelShader;

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\CubeMapPreFilter.fx", "VS", &pVertexShader, NXGlobalInputLayout::layoutPNT, ARRAYSIZE(NXGlobalInputLayout::layoutPNT), &pInputLayoutP);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\CubeMapPreFilter.fx", "PS", &pPixelShader);
	g_pContext->VSSetShader(pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(pInputLayoutP.Get());

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferObject);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	ComPtr<ID3D11Buffer> cbCubeCamera;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &cbCubeCamera));

	ComPtr<ID3D11Buffer> cbRoughness;
	bufferDesc.ByteWidth = sizeof(ConstantBufferObject);
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &cbRoughness));

	g_pContext->PSSetShaderResources(0, 1, m_pSRVCubeMap.GetAddressOf());
	g_pContext->VSSetConstantBuffers(0, 1, cbCubeCamera.GetAddressOf());
	g_pContext->PSSetConstantBuffers(1, 1, cbRoughness.GetAddressOf());

	ConstantBufferObject cbDataCubeCamera;
	cbDataCubeCamera.world = Matrix::Identity();
	cbDataCubeCamera.projection = m_mxCubeMapProj.Transpose();

	UINT stride = sizeof(VertexP);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, m_pVertexBufferCubeBox.GetAddressOf(), &stride, &offset);
	g_pContext->IASetIndexBuffer(m_pIndexBufferCubeBox.Get(), DXGI_FORMAT_R32_UINT, 0);

	float roughValues[5] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
	ConstantBufferFloat cbDataRoughness;
	cbDataRoughness.value = 0;

	UINT uMapSize = (UINT)MapSize;
	CD3D11_VIEWPORT vp;
	for (int i = 0; i < 5; i++)
	{
		cbDataRoughness.value = roughValues[i] * roughValues[i];

		vp = CD3D11_VIEWPORT(0.0f, 0.0f, (float)(uMapSize >> i), (float)(uMapSize >> i));
		g_pContext->RSSetViewports(1, &vp);
		for (int j = 0; j < 6; j++)
		{
			cbDataCubeCamera.view = m_mxCubeMapView[j].Transpose();
			g_pContext->ClearRenderTargetView(m_pRTVPreFilterMaps[i][j].Get(), Colors::WhiteSmoke);
			g_pContext->OMSetRenderTargets(1, m_pRTVPreFilterMaps[i][j].GetAddressOf(), nullptr);

			g_pContext->UpdateSubresource(cbCubeCamera.Get(), 0, nullptr, &cbDataCubeCamera, 0, 0);
			g_pContext->UpdateSubresource(cbRoughness.Get(), 0, nullptr, &cbDataRoughness, 0, 0);
			g_pContext->DrawIndexed((UINT)m_indicesCubeBox.size() / 6, j * 6, 0);
		}
	}

	g_pUDA->EndEvent();
}

void NXCubeMap::GenerateBRDF2DLUT()
{
	g_pUDA->BeginEvent(L"Generate BRDF 2D LUT");

	NXSamplerState<D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP>::Create();

	const static float MapSize = 512.0f;
	CD3D11_VIEWPORT vp(0.0f, 0.0f, MapSize, MapSize);
	g_pContext->RSSetViewports(1, &vp);

	CD3D11_TEXTURE2D_DESC descTex(DXGI_FORMAT_R8G8B8A8_UNORM, (UINT)MapSize, (UINT)MapSize, 1, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, 1, 0, 0);
	g_pDevice->CreateTexture2D(&descTex, nullptr, &m_pTexBRDF2DLUT);

	CD3D11_SHADER_RESOURCE_VIEW_DESC descSRV(D3D11_SRV_DIMENSION_TEXTURE2D, descTex.Format, 0, descTex.MipLevels, 0, descTex.ArraySize);
	g_pDevice->CreateShaderResourceView(m_pTexBRDF2DLUT.Get(), &descSRV, &m_pSRVBRDF2DLUT);

	CD3D11_RENDER_TARGET_VIEW_DESC descRTV(D3D11_RTV_DIMENSION_TEXTURE2D, descTex.Format);
	g_pDevice->CreateRenderTargetView(m_pTexBRDF2DLUT.Get(), &descRTV, &m_pRTVBRDF2DLUT);

	std::vector<VertexPT> vertices =
	{
		// +Z
		{ Vector3(+1.0f, +1.0f, +1.0f), Vector2(1.0f, 1.0f) },
		{ Vector3(-1.0f, +1.0f, +1.0f), Vector2(1.0f, 0.0f) },
		{ Vector3(-1.0f, -1.0f, +1.0f), Vector2(0.0f, 0.0f) },
		{ Vector3(+1.0f, -1.0f, +1.0f), Vector2(0.0f, 1.0f) },
	};
	 
	std::vector<UINT> indices =
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

	ComPtr<ID3D11Buffer> pVertexBuffer;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &pVertexBuffer));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(UINT) * (UINT)indices.size();
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	InitData.pSysMem = indices.data();

	ComPtr<ID3D11Buffer> pIndexBuffer;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &pIndexBuffer));

	ComPtr<ID3D11InputLayout> pInputLayoutPT;
	ComPtr<ID3D11VertexShader> pVertexShader;
	ComPtr<ID3D11PixelShader> pPixelShader;

	NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\BRDF2DLUT.fx", "VS", &pVertexShader, NXGlobalInputLayout::layoutPT, ARRAYSIZE(NXGlobalInputLayout::layoutPT), &pInputLayoutPT);
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\BRDF2DLUT.fx", "PS", &pPixelShader);
	g_pContext->VSSetShader(pVertexShader.Get(), nullptr, 0);
	g_pContext->PSSetShader(pPixelShader.Get(), nullptr, 0);
	g_pContext->IASetInputLayout(pInputLayoutPT.Get());

	UINT stride = sizeof(VertexPT); 
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, pVertexBuffer.GetAddressOf(), &stride, &offset);
	g_pContext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	g_pContext->ClearRenderTargetView(m_pRTVBRDF2DLUT.Get(), Colors::WhiteSmoke);
	g_pContext->OMSetRenderTargets(1, m_pRTVBRDF2DLUT.GetAddressOf(), nullptr);
	g_pContext->DrawIndexed((UINT)indices.size(), 0, 0);

	g_pUDA->EndEvent();
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

	m_verticesCubeBox =
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

	m_indicesCubeBox =
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

	UpdateVBIB();
}

void NXCubeMap::UpdateVBIB()
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

	bufferDesc.ByteWidth = sizeof(VertexP) * (UINT)m_verticesCubeBox.size();
	InitData.pSysMem = m_verticesCubeBox.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pVertexBufferCubeBox));

	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(UINT) * (UINT)m_indices.size();
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;

	InitData.pSysMem = m_indices.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pIndexBuffer));

	bufferDesc.ByteWidth = sizeof(UINT) * (UINT)m_indicesCubeBox.size();
	InitData.pSysMem = m_indicesCubeBox.data();
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, &InitData, &m_pIndexBufferCubeBox));
}

void NXCubeMap::InitConstantBuffer()
{
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(ConstantBufferCubeMap);
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &m_cb));
}
