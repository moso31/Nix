#include "NXCubeMap.h"
#include "SphereHarmonics.h"
#include "Global.h"
#include "GlobalBufferManager.h"
#include "NXScene.h"
#include "NXCamera.h"
#include "ShaderComplier.h"
#include "DirectResources.h"
#include "NXRenderStates.h"
#include "NXSamplerStates.h"
#include "NXResourceManager.h"
#include "DirectXTex.h"
#include "NXConverter.h"
#include "SamplerMath.h"
#include "NXTexture.h"
#include "NXSubMeshGeometryEditor.h"

using namespace DirectX::SamplerMath;
using namespace DirectX::SimpleMath::SH;

NXCubeMap::NXCubeMap(NXScene* pScene) :
	m_pScene(pScene)
{
	m_cbAllocator = new CommittedAllocator(g_pDevice.Get(), 256);
	m_cbAllocator->Alloc(ResourceType_Upload, m_cbData);
}

bool NXCubeMap::Init(const std::filesystem::path& filePath)
{
	NX12Util::CreateCommands(g_pDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandQueue.Get(), m_pCommandAllocator.Get(), m_pCommandList.Get());

	// create vertex
	InitVertex();
	InitRootSignature();

	m_mxCubeMapProj = XMMatrixPerspectiveFovLH(XMConvertToRadians(90.0f), 1.0f, 0.1f, 10.0f);
	m_mxCubeMapView[0] = XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
	m_mxCubeMapView[1] = XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
	m_mxCubeMapView[2] = XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f));
	m_mxCubeMapView[3] = XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f));
	m_mxCubeMapView[4] = XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f));
	m_mxCubeMapView[5] = XMMatrixLookAtLH(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f));

	std::string strExtension = NXConvert::s2lower(filePath.extension().string().c_str());

	if (strExtension == ".dds")
	{
		LoadDDS(filePath);
	}
	else if (strExtension == ".hdr")
	{
		// 1. HDR纹理
		Ntr<NXTexture2D> pTexHDR = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("HDR Temp Texture", filePath);
		pTexHDR->AddSRV();

		// 2. 先使用HDR->DDS，然后将DDS保存为本地文件，再读取DDS本地文件作为实际CubeMap。
		// 直接用HDR->DDS然后作为CubeMap的话，不知道什么原因GPU队列会严重阻塞，导致加载速度大幅减慢。

		Ntr<NXTextureCube> pTexCubeMap = GenerateCubeMap(pTexHDR);
		SaveHDRAsDDS(pTexCubeMap, filePath);
		LoadDDS(filePath);
	}

	GenerateIrradianceSHFromCubeMap();
	GeneratePreFilterMap();

	return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE NXCubeMap::GetSRVCubeMap()
{
	return m_pTexCubeMap->GetSRV();
}

D3D12_CPU_DESCRIPTOR_HANDLE NXCubeMap::GetSRVCubeMapPreview2D()
{
	return m_pTexCubeMap->GetSRVPreview2D();
}

D3D12_CPU_DESCRIPTOR_HANDLE NXCubeMap::GetSRVIrradianceMap()
{
	return m_pTexIrradianceMap->GetSRV();
}

D3D12_CPU_DESCRIPTOR_HANDLE NXCubeMap::GetSRVPreFilterMap()
{
	return m_pTexPreFilterMap->GetSRV();
}

void NXCubeMap::Update()
{
	m_cbAllocator->UpdateData(m_cbData);
}

void NXCubeMap::UpdateViewParams()
{
	auto pCamera = m_pScene->GetMainCamera();

	auto& cbDataObject = NXGlobalBufferManager::m_cbDataObject.Current();
	cbDataObject.data.world = Matrix::CreateTranslation(pCamera->GetTranslation()).Transpose();
	m_cbAllocator->UpdateData(cbDataObject);
}

void NXCubeMap::Render(ID3D12GraphicsCommandList* pCmdList)
{
	const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews("_CubeMapSphere");
	pCmdList->IASetVertexBuffers(0, 1, &meshView.vbv);
	pCmdList->IASetIndexBuffer(&meshView.ibv);
	pCmdList->DrawIndexedInstanced((UINT)meshView.indexCount, 1, 0, 0, 0);
}

void NXCubeMap::Release()
{
	NXTransform::Release();
}

Ntr<NXTextureCube> NXCubeMap::GenerateCubeMap(Ntr<NXTexture2D>& pTexHDR)
{
	PIXBeginEvent(m_pCommandList.Get(), 0, L"Generate Cube Map");

	auto pGlobalDescriptorAllocator = NXAllocatorManager::GetInstance()->GetDescriptorAllocator();

	const static float mapSize = 1024;
	auto vp = NX12Util::ViewPort(mapSize, mapSize);
	m_pCommandList->RSSetViewports(1, &vp);

	Ntr<NXTextureCube> pTexCubeMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTextureCube("Main CubeMap", DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)mapSize, (UINT)mapSize, 1);

	pTexCubeMap->AddSRV();
	for (int i = 0; i < 6; i++) pTexCubeMap->AddRTV(0, i, 1);

	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\HDRToCubeMap.fx", "VS", pVSBlob.Get());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\HDRToCubeMap.fx", "PS", pPSBlob.Get());

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSigCubeMap.Get();
	psoDesc.InputLayout = { NXGlobalInputLayout::layoutP, 1 };
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = pTexCubeMap->GetFormat();
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	g_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSOCubeMap));

	CommittedResourceData<ConstantBufferObject> cbData;
	cbData.data.world = Matrix::Identity();
	cbData.data.projection = m_mxCubeMapProj.Transpose();
	m_cbAllocator->Alloc(ResourceType_Upload, cbData);

	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle;
	pGlobalDescriptorAllocator->Alloc(DescriptorType_CBV, cbvHandle);
	g_pDevice->CreateConstantBufferView(&cbData.CBVDesc(), cbvHandle);

	m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto pShaderVisibleDescriptorHeap = NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorHeap();
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = pShaderVisibleDescriptorHeap->Append(pTexHDR->GetSRVArray(), pTexHDR->GetSRVs());

	ID3D12DescriptorHeap* ppHeaps[] = { pShaderVisibleDescriptorHeap->GetHeap() };
	m_pCommandList->SetDescriptorHeaps(1, ppHeaps);

	m_pCommandList->SetGraphicsRootSignature(m_pRootSigCubeMap.Get());
	m_pCommandList->SetPipelineState(m_pPSOCubeMap.Get());

	pTexCubeMap->SetResourceState(m_pCommandList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	for (int i = 0; i < 6; i++)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pTexCubeMap->GetRTV(i);
		m_pCommandList->ClearRenderTargetView(rtvHandle, Colors::WhiteSmoke, 0, nullptr);
		m_pCommandList->OMSetRenderTargets(1, &rtvHandle, true, nullptr);

		cbData.data.view = m_mxCubeMapView[i].Transpose();
		m_cbAllocator->UpdateData(cbData);

		m_pCommandList->SetGraphicsRootConstantBufferView(0, cbData.GPUVirtualAddr);
		m_pCommandList->SetGraphicsRootDescriptorTable(1, srvHandle);

		const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews("_CubeMapBox");
		m_pCommandList->IASetVertexBuffers(0, 1, &meshView.vbv);
		m_pCommandList->IASetIndexBuffer(&meshView.ibv);
		m_pCommandList->DrawIndexedInstanced(6, 1, i * 6, 0, 0);
	}

	PIXEndEvent();
	return pTexCubeMap;
}

void NXCubeMap::GenerateIrradianceSHFromHDRI(Ntr<NXTexture2D>& pTexHDR)
{
	TexMetadata metadata;
	const std::wstring& strFilePath = pTexHDR->GetFilePath();
	std::unique_ptr<ScratchImage> pHDRImage = std::make_unique<ScratchImage>();

	LoadFromHDRFile(strFilePath.c_str(), &metadata, *pHDRImage);

	bool bResize = true;
	if (bResize)
	{
		std::unique_ptr<ScratchImage> timage = std::make_unique<ScratchImage>();
		HRESULT hr = Resize(pHDRImage->GetImages(), pHDRImage->GetImageCount(), pHDRImage->GetMetadata(), 256, 128, TEX_FILTER_DEFAULT, *timage);
		if (SUCCEEDED(hr))
		{
			auto& tinfo = timage->GetMetadata();

			metadata.width = tinfo.width;
			metadata.height = tinfo.height;
			metadata.mipLevels = 1;

			pHDRImage.swap(timage);
		}
		else
		{
			printf("Warning: [Resize] failure when loading NXTextureCube file: %ws\n", strFilePath.c_str());
		}
	}

	size_t imgWidth = metadata.width;
	size_t imgHeight = metadata.height;

	// HDRI 纹理加载
	auto pData = reinterpret_cast<float*>(pHDRImage->GetImage(0, 0, 0)->pixels);
	float solidAnglePdf = 0.0;
	memset(m_shIrradianceMap_CPU, 0, sizeof(m_shIrradianceMap_CPU));

	// 像素个数
	size_t pixelCount = pHDRImage->GetPixelsSize() >> 4;

	size_t threadCount = imgHeight;
	for (int threadIdx = 0; threadIdx < (int)threadCount; threadIdx++)
	{
		size_t threadSize = pixelCount / threadCount;
		for (int i = 0; i < threadSize; i++)
		{
			size_t idx = threadIdx * threadSize + i;

			float u = (float(idx % imgWidth) + 0.5f) / imgWidth;
			float v = (float(idx / imgWidth) + 0.5f) / imgHeight;

			float scaleY = 0.5f / imgHeight;
			float thetaU = (v - scaleY) * XM_PI;
			float thetaD = (v + scaleY) * XM_PI;

			float dPhi = XM_2PI / imgWidth;	// dPhi 是个常量
			float dTheta = cos(thetaU) - cos(thetaD);
			solidAnglePdf = dPhi * dTheta;

			auto theta = v * XM_PI;
			auto phi = (u - 0.25f) * XM_2PI;

			// get L(Rs).
			size_t offset = idx << 2;
			Vector3 pixel(pData + offset);

			for (int l = 0; l < 3; l++)
			{
				for (int m = -l; m <= l; m++)
				{
					float sh = SHBasis(l, m, theta, phi);  // HDRI纹理角度矫正

					// sh = y_l^m(Rs)
					// m_shIrradianceMap[k++] = L_l^m
					Vector3 Llm = pixel * sh * solidAnglePdf;

					//printf("%d | %d, %d | %f, %f, %f\n", idx, l, m, Llm.x, Llm.y, Llm.z);
					{
						m_shIrradianceMap_CPU[l * l + l + m] += Llm;
					}
				}
			}

			//printf("\n");
		}
	}

	const float T[5] = { 0.886226925452757f, 1.023326707946489f, 0.495415912200751f, 0.0f, -0.110778365681594f };
	int k = 0;
	for (int l = 0; l < 3; l++)
	{
		for (int m = -l; m <= l; m++)
		{
			// 求 E_l^m
			m_shIrradianceMap_CPU[k++] *= sqrt(XM_4PI / (2.0f * l + 1.0f)) * T[l] * XM_1DIVPI;
		}
	}
}

void NXCubeMap::GenerateIrradianceSHFromCubeMap()
{
	// 2023.3.15 按现阶段对NXResourceManager的设计，是没办法拿到具体每个纹理像素的值的
	// 所以暂时先手动控制Irradiance的加载逻辑
	size_t nIrradTexSize = 128;
	const std::wstring& strFilePath = m_pTexCubeMap->GetFilePath();
	TexMetadata metadata;
	std::unique_ptr<ScratchImage> pImage = std::make_unique<ScratchImage>();

	LoadFromDDSFile(strFilePath.c_str(), DDS_FLAGS_NONE, &metadata, *pImage);

	if (IsCompressed(metadata.format))
	{
		auto img = pImage->GetImage(0, 0, 0);
		size_t nimg = pImage->GetImageCount();

		std::unique_ptr<ScratchImage> dcImage = std::make_unique<ScratchImage>();
		HRESULT hr = Decompress(img, nimg, metadata, DXGI_FORMAT_UNKNOWN /* picks good default */, *dcImage);
		if (SUCCEEDED(hr))
		{
			if (dcImage && dcImage->GetPixels())
				pImage.swap(dcImage);
		}
		else
		{
			printf("Warning: [Decompress] failure when loading NXTextureCube file: %ws\n", strFilePath.c_str());
		}
	}

	bool bResize = nIrradTexSize < metadata.width && nIrradTexSize < metadata.height;
	if (bResize)
	{
		std::unique_ptr<ScratchImage> timage = std::make_unique<ScratchImage>();
		HRESULT hr = Resize(pImage->GetImages(), pImage->GetImageCount(), pImage->GetMetadata(), nIrradTexSize, nIrradTexSize, TEX_FILTER_DEFAULT, *timage);
		if (SUCCEEDED(hr))
		{
			auto& tinfo = timage->GetMetadata();

			metadata.width = tinfo.width;
			metadata.height = tinfo.height;
			metadata.mipLevels = 1;

			pImage.swap(timage);
		}
		else
		{
			printf("Warning: [Resize] failure when loading NXTextureCube file: %ws\n", strFilePath.c_str());
		}
	}

	memset(m_shIrradianceMap, 0, sizeof(m_shIrradianceMap));
	double test = 0.0;
	double solidAnglePdfsum = 0.0;
	for (UINT iFace = 0; iFace < 6; iFace++)
	{
		auto pData = reinterpret_cast<float*>(pImage->GetImage(0, iFace, 0)->pixels);
		for (UINT i = 0; i < nIrradTexSize; i++)
		{
			for (UINT j = 0; j < nIrradTexSize; j++)
			{
				UINT idx = i * (UINT)nIrradTexSize + j;

				// note: u, v = (-1 .. 1)
				float u = (float)(j + 0.5f) / (float)nIrradTexSize * 2.0f - 1.0f;
				float v = (float)(i + 0.5f) / (float)nIrradTexSize * 2.0f - 1.0f;

				Vector3 dir;
				if (iFace == 0) dir = Vector3(1.0f, -v, -u);
				if (iFace == 1) dir = Vector3(-1.0f, -v, u);
				if (iFace == 2) dir = Vector3(u, 1.0f, v);
				if (iFace == 3) dir = Vector3(u, -1.0f, -v);
				if (iFace == 4) dir = Vector3(u, -v, 1.0f);
				if (iFace == 5) dir = Vector3(-u, -v, -1.0f);
				dir.Normalize();

				float theta = acosf(dir.y);
				auto k = atan2f(dir.z, dir.x);
				float phi = XM_PIDIV2 - atan2f(dir.z, dir.x);

				// 4byte = 32bit. 
				// a R32G32B32A32 pixel = 16byte = 128bit.
				size_t offset = idx << 2;

				// get L(Rs).
				Vector3 pixel(pData + offset);

				float solidAnglePdf = CubeMapSolidAngleOfPixel(i, j, nIrradTexSize);
				solidAnglePdfsum += solidAnglePdf;

				for (int l = 0; l < 3; l++)
				{
					for (int m = -l; m <= l; m++)
					{
						float sh = SHBasis(l, m, theta, phi);  // HDRI纹理角度矫正

						// sh = y_l^m(Rs)
						// m_shIrradianceMap[k++] = L_l^m
						Vector3 Llm = pixel * sh * solidAnglePdf;
						//printf("%d | %d, %d | %f, %f, %f\n", idx, l, m, Llm.x, Llm.y, Llm.z);

						{
							m_shIrradianceMap[l * l + l + m] += Llm;
						}
					}
				}
			}
		}
	}

	const float T[5] = { 0.886226925452757f, 1.023326707946489f, 0.495415912200751f, 0.0f, -0.110778365681594f };
	int k = 0;
	for (int l = 0; l < 3; l++)
	{
		for (int m = -l; m <= l; m++)
		{
			// 求 E_l^m
			m_shIrradianceMap[k++] *= sqrt(XM_4PI / (2.0f * l + 1.0f)) * T[l] * XM_1DIVPI;
		}
	}

	m_cbData.data.irradSH0123x = Vector4(m_shIrradianceMap[0].x, m_shIrradianceMap[1].x, m_shIrradianceMap[2].x, m_shIrradianceMap[3].x);
	m_cbData.data.irradSH0123y = Vector4(m_shIrradianceMap[0].y, m_shIrradianceMap[1].y, m_shIrradianceMap[2].y, m_shIrradianceMap[3].y);
	m_cbData.data.irradSH0123z = Vector4(m_shIrradianceMap[0].z, m_shIrradianceMap[1].z, m_shIrradianceMap[2].z, m_shIrradianceMap[3].z);
	m_cbData.data.irradSH4567x = Vector4(m_shIrradianceMap[4].x, m_shIrradianceMap[5].x, m_shIrradianceMap[6].x, m_shIrradianceMap[7].x);
	m_cbData.data.irradSH4567y = Vector4(m_shIrradianceMap[4].y, m_shIrradianceMap[5].y, m_shIrradianceMap[6].y, m_shIrradianceMap[7].y);
	m_cbData.data.irradSH4567z = Vector4(m_shIrradianceMap[4].z, m_shIrradianceMap[5].z, m_shIrradianceMap[6].z, m_shIrradianceMap[7].z);
	m_cbData.data.irradSH8xyz = m_shIrradianceMap[8];
	int x = 0;
}

// ps. DX11 升级 DX12 期间暂时禁用
void NXCubeMap::GenerateIrradianceMap()
{
	//g_pUDA->BeginEvent(L"Generate Irradiance Map");

	//auto pSampler = NXSamplerManager::Get(NXSamplerFilter::Linear, NXSamplerAddressMode::Wrap);
	//g_pContext->PSSetSamplers(0, 1, &pSampler);

	//const static float MapSize = 32.0f;
	//CD3D11_VIEWPORT vp(0.0f, 0.0f, MapSize, MapSize);
	//g_pContext->RSSetViewports(1, &vp);

	//m_pTexIrradianceMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTextureCube("Irradiance IBL Tex", m_pTexCubeMap->GetFormat(), (UINT)MapSize, (UINT)MapSize, 1, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DEFAULT, 0, 1, 0, D3D11_RESOURCE_MISC_TEXTURECUBE);
	//m_pTexIrradianceMap->AddSRV();
	//for (int i = 0; i < 6; i++)
	//	m_pTexIrradianceMap->AddRTV(0, i, 1);

	//ComPtr<ID3D11InputLayout> pInputLayoutP;
	//ComPtr<ID3D11VertexShader> pVertexShader;
	//ComPtr<ID3D11PixelShader> pPixelShader;

	//NXShaderComplier::GetInstance()->CompileVSIL(L"Shader\\CubeMapIrradiance.fx", "VS", &pVertexShader, NXGlobalInputLayout::layoutPNT, ARRAYSIZE(NXGlobalInputLayout::layoutPNT), &pInputLayoutP);
	//NXShaderComplier::GetInstance()->CompilePS(L"Shader\\CubeMapIrradiance.fx", "PS", &pPixelShader);
	//g_pContext->VSSetShader(pVertexShader.Get(), nullptr, 0);
	//g_pContext->PSSetShader(pPixelShader.Get(), nullptr, 0);
	//g_pContext->IASetInputLayout(pInputLayoutP.Get());

	//ComPtr<ID3D11Buffer> cb;
	//D3D11_BUFFER_DESC bufferDesc;
	//ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	//bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	//bufferDesc.ByteWidth = sizeof(ConstantBufferObject);
	//bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	//bufferDesc.CPUAccessFlags = 0;
	//NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &cb));

	//auto pSRVCubeMap = m_pTexCubeMap->GetSRV();
	//g_pContext->PSSetShaderResources(0, 1, &pSRVCubeMap);
	//g_pContext->VSSetConstantBuffers(0, 1, cb.GetAddressOf());

	//ConstantBufferObject cbData;
	//cbData.world = Matrix::Identity();
	//cbData.projection = m_mxCubeMapProj.Transpose();

	//UINT stride = sizeof(VertexP);
	//UINT offset = 0;
	//g_pContext->IASetVertexBuffers(0, 1, m_pVertexBufferCubeBox.GetAddressOf(), &stride, &offset);
	//g_pContext->IASetIndexBuffer(m_pIndexBufferCubeBox.Get(), DXGI_FORMAT_R32_UINT, 0);

	//for (int i = 0; i < 6; i++)
	//{
	//	auto pRTV = m_pTexIrradianceMap->GetRTV(i);
	//	g_pContext->ClearRenderTargetView(pRTV, Colors::WhiteSmoke);
	//	g_pContext->OMSetRenderTargets(1, &pRTV, nullptr);

	//	cbData.view = m_mxCubeMapView[i].Transpose();
	//	g_pContext->UpdateSubresource(cb.Get(), 0, nullptr, &cbData, 0, 0);
	//	g_pContext->DrawIndexed((UINT)m_indicesCubeBox.size() / 6, i * 6, 0);
	//}

	//g_pUDA->EndEvent();
}

void NXCubeMap::GeneratePreFilterMap()
{
	NX12Util::BeginEvent(m_pCommandList.Get(), "Generate PreFilter Map");

	auto pGlobalDescriptorAllocator = NXAllocatorManager::GetInstance()->GetDescriptorAllocator();

	float mapSize = 512.0f;
	m_pTexPreFilterMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTextureCube("PreFilter Map", m_pTexCubeMap->GetFormat(), (UINT)mapSize, (UINT)mapSize, 5);
	m_pTexPreFilterMap->AddSRV();

	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 6; j++)
		{
			// mipSize = i, firstarray = j, arraysize = 1
			m_pTexPreFilterMap->AddRTV(i, j, 1);
		}
	}

	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\CubeMapPreFilter.fx", "VS", pVSBlob.Get());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\CubeMapPreFilter.fx", "PS", pPSBlob.Get());

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSigPreFilterMap.Get();
	psoDesc.InputLayout = { NXGlobalInputLayout::layoutPNT, 1 };
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_pTexPreFilterMap->GetFormat();
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	g_pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSOPreFilterMap));

	CommittedResourceData<ConstantBufferObject> cbCubeCamera;
	cbCubeCamera.data.world = Matrix::Identity();
	cbCubeCamera.data.projection = m_mxCubeMapProj.Transpose();
	m_cbAllocator->Alloc(ResourceType_Upload, cbCubeCamera);

	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle;
	pGlobalDescriptorAllocator->Alloc(DescriptorType_CBV, cbvHandle);
	g_pDevice->CreateConstantBufferView(&cbCubeCamera.CBVDesc(), cbvHandle);

	m_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	auto pShaderVisibleDescriptorHeap = NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorHeap();
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = pShaderVisibleDescriptorHeap->Append(m_pTexCubeMap->GetSRVArray(), m_pTexCubeMap->GetSRVs());

	ID3D12DescriptorHeap* ppHeaps[] = { pShaderVisibleDescriptorHeap->GetHeap() };
	m_pCommandList->SetDescriptorHeaps(1, ppHeaps);

	m_pCommandList->SetGraphicsRootSignature(m_pRootSigPreFilterMap.Get());
	m_pCommandList->SetPipelineState(m_pPSOPreFilterMap.Get());

	float rough[] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };
	for (int i = 0; i < _countof(rough); i++)
	{
		float mipSize = (float)((UINT)mapSize >> i);

		CommittedResourceData<ConstantBufferFloat> cbRoughness;
		cbRoughness.data.value = rough[i] * rough[i];
		m_cbAllocator->UpdateData(cbRoughness);

		D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle;
		pGlobalDescriptorAllocator->Alloc(DescriptorType_CBV, cbvHandle);
		g_pDevice->CreateConstantBufferView(&cbRoughness.CBVDesc(), cbvHandle);

		auto vp = NX12Util::ViewPort(mipSize, mipSize);
		m_pCommandList->RSSetViewports(1, &vp);

		for (int j = 0; j < 6; j++)
		{
			cbCubeCamera.data.view = m_mxCubeMapView[j].Transpose();

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pTexPreFilterMap->GetRTV(i * 6 + j);
			m_pCommandList->ClearRenderTargetView(rtvHandle, Colors::WhiteSmoke, 0, nullptr);
			m_pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
			m_cbAllocator->UpdateData(cbCubeCamera);

			m_pCommandList->SetGraphicsRootConstantBufferView(0, cbCubeCamera.GPUVirtualAddr);
			m_pCommandList->SetGraphicsRootConstantBufferView(1, cbRoughness.GPUVirtualAddr);
			m_pCommandList->SetGraphicsRootDescriptorTable(2, srvHandle);

			const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews("_CubeMapBox");
			m_pCommandList->IASetVertexBuffers(0, 1, &meshView.vbv);
			m_pCommandList->IASetIndexBuffer(&meshView.ibv);
			m_pCommandList->DrawIndexedInstanced(6, 1, i * 6, 0, 0);
		}
	}

	NX12Util::EndEvent();
}

void NXCubeMap::SaveHDRAsDDS(Ntr<NXTextureCube>& pTexCubeMap, const std::filesystem::path& filePath)
{
	// Save as *.dds texture file.
	std::unique_ptr<ScratchImage> pMappedImage = std::make_unique<ScratchImage>();
	HRESULT hr = CaptureTexture(m_pCommandQueue.Get(), pTexCubeMap->GetTex(), true, *pMappedImage);
	TexMetadata cubeDDSInfo = pMappedImage->GetMetadata();

	std::filesystem::path strPath = filePath;
	strPath.replace_extension(".dds");
	hr = SaveToDDSFile(pMappedImage->GetImages(), pMappedImage->GetImageCount(), cubeDDSInfo, DDS_FLAGS_NONE, strPath.wstring().c_str());
}

void NXCubeMap::LoadDDS(const std::filesystem::path& filePath)
{
	std::filesystem::path strPath = filePath;
	strPath.replace_extension(".dds");

	m_pTexCubeMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTextureCube("CubeMap Texture", strPath.wstring());
	m_pTexCubeMap->AddSRV();
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

	NXSubMeshGeometryEditor::GetInstance()->CreateVBIB(m_vertices, m_indices, "_CubeMapSphere");
	NXSubMeshGeometryEditor::GetInstance()->CreateVBIB(m_verticesCubeBox, m_indicesCubeBox, "_CubeMapBox");
}

void NXCubeMap::InitRootSignature()
{
	std::vector<D3D12_STATIC_SAMPLER_DESC> pSamplers;
	pSamplers.push_back(NXStaticSamplerState<>::Create(0, 0, D3D12_SHADER_VISIBILITY_ALL)); // s0

	// cubemap
	std::vector<D3D12_DESCRIPTOR_RANGE> rangesCubeMap;
	rangesCubeMap.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0)); // t0 ~ t0.

	std::vector<D3D12_ROOT_PARAMETER> rootParamsCubeMap;
	rootParamsCubeMap.push_back(NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL)); // b0
	rootParamsCubeMap.push_back(NX12Util::CreateRootParameterTable(rangesCubeMap, D3D12_SHADER_VISIBILITY_ALL)); // 上面的 rangesCubeMap. t0 ~ t0.

	m_pRootSigCubeMap = NX12Util::CreateRootSignature(g_pDevice.Get(), rootParamsCubeMap, pSamplers);

	// prefilter map
	std::vector<D3D12_DESCRIPTOR_RANGE> rangesPreFilter;
	rangesPreFilter.push_back(NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0)); // t0 ~ t0.

	std::vector<D3D12_ROOT_PARAMETER> rootParamsPreFilter;
	rootParamsPreFilter.push_back(NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL)); // b0
	rootParamsPreFilter.push_back(NX12Util::CreateRootParameterCBV(1, 0, D3D12_SHADER_VISIBILITY_ALL)); // b1
	rootParamsPreFilter.push_back(NX12Util::CreateRootParameterTable(rangesPreFilter, D3D12_SHADER_VISIBILITY_ALL)); // 上面的 rangesPreFilter. t0 ~ t0.

	m_pRootSigPreFilterMap = NX12Util::CreateRootSignature(g_pDevice.Get(), rootParamsPreFilter, pSamplers);
}

////////////////////////////////////////////////////////////////////////////
//// Deprecated functions...
////////////////////////////////////////////////////////////////////////////

void NXCubeMap::GenerateIrradianceSHFromHDRI_Deprecated(NXTexture2D* pTexHDR)
{
	//g_pUDA->BeginEvent(L"Generate Irradiance Map SH");

	//auto pSampler = NXSamplerManager::Get(NXSamplerFilter::Linear, NXSamplerAddressMode::Clamp);
	//g_pContext->CSSetSamplers(0, 1, &pSampler);

	//ComPtr<ID3D11Buffer> cbImageSize;
	//D3D11_BUFFER_DESC bufferDesc;
	//ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	//bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	//bufferDesc.ByteWidth = sizeof(ConstantBufferImageData);
	//bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	//bufferDesc.CPUAccessFlags = 0;
	//NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &cbImageSize));

	//UINT nThreadSize = 8;
	//UINT tempWidth = pTexHDR->GetWidth();
	//UINT tempHeight = pTexHDR->GetHeight();
	//UINT SHIrradPassCount = 0;
	//while (tempWidth != 1 || tempHeight != 1)
	//{
	//	tempWidth = max((tempWidth + nThreadSize - 1) / nThreadSize, 1);
	//	tempHeight = max((tempHeight + nThreadSize - 1) / nThreadSize, 1);
	//	SHIrradPassCount++;
	//}
	//SHIrradPassCount = max(SHIrradPassCount, 2);

	//tempWidth = pTexHDR->GetWidth();
	//tempHeight = pTexHDR->GetHeight();
	//std::vector<ComPtr<ID3D11ShaderResourceView>> pSRVIrradSHs;
	//pSRVIrradSHs.reserve(SHIrradPassCount);
	//for (UINT passId = 0; passId < SHIrradPassCount; passId++)
	//{
	//	std::wstring eventName = L"SH Irradiance Gather" + std::to_wstring(passId);
	//	g_pUDA->BeginEvent(eventName.c_str());

	//	UINT currWidth = tempWidth;
	//	UINT currHeight = tempHeight;

	//	tempWidth = max((tempWidth + nThreadSize - 1) / nThreadSize, 1);
	//	tempHeight = max((tempHeight + nThreadSize - 1) / nThreadSize, 1);

	//	ConstantBufferImageData cbImageSizeData;
	//	cbImageSizeData.currImgSize = Vector4((float)currWidth, (float)currHeight, 1.0f / (float)currWidth, 1.0f / (float)currHeight);
	//	cbImageSizeData.nextImgSize = Vector4((float)tempWidth, (float)tempHeight, 1.0f / (float)tempWidth, 1.0f / (float)tempHeight);

	//	g_pContext->UpdateSubresource(cbImageSize.Get(), 0, nullptr, &cbImageSizeData, 0, 0);
	//	g_pContext->CSSetConstantBuffers(0, 1, cbImageSize.GetAddressOf());

	//	size_t irradianceBufferElements = tempWidth * tempHeight;
	//	size_t irradianceBufferSize = sizeof(ConstantBufferIrradSH) * irradianceBufferElements;

	//	D3D11_BUFFER_DESC bufferDesc;
	//	ComPtr<ID3D11Buffer> cbIrradianceSH;
	//	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	//	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	//	bufferDesc.ByteWidth = (UINT)irradianceBufferSize;
	//	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	//	bufferDesc.CPUAccessFlags = 0;
	//	bufferDesc.StructureByteStride = sizeof(ConstantBufferIrradSH);
	//	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	//	NX::ThrowIfFailed(g_pDevice->CreateBuffer(&bufferDesc, nullptr, &cbIrradianceSH));

	//	ComPtr<ID3D11ComputeShader> pComputeShader;
	//	std::wstring strCSPath = L"";

	//	// 设置 CubeMapIrradianceSH.fx 使用哪个入口点函数
	//	if (passId == 0)
	//	{
	//		CD3D_SHADER_MACRO macro("CUBEMAP_IRRADSH_FIRST", "1");
	//		NXShaderComplier::GetInstance()->AddMacro(macro);
	//	}
	//	else if (passId > 0 && passId < SHIrradPassCount - 1)
	//	{
	//		CD3D_SHADER_MACRO macro("CUBEMAP_IRRADSH_MIDDLE", "1");
	//		NXShaderComplier::GetInstance()->AddMacro(macro);
	//	}
	//	else
	//	{
	//		CD3D_SHADER_MACRO macro("CUBEMAP_IRRADSH_LAST", "1");
	//		NXShaderComplier::GetInstance()->AddMacro(macro);
	//	}

	//	NXShaderComplier::GetInstance()->CompileCS(L"Shader\\CubeMapIrradianceSH.fx", "CS", &pComputeShader);

	//	ComPtr<ID3D11UnorderedAccessView> pUAVIrradSH;
	//	CD3D11_UNORDERED_ACCESS_VIEW_DESC UAVDesc(D3D11_UAV_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, 0, (UINT)irradianceBufferElements);
	//	NX::ThrowIfFailed(g_pDevice->CreateUnorderedAccessView(cbIrradianceSH.Get(), &UAVDesc, pUAVIrradSH.GetAddressOf()));

	//	ComPtr<ID3D11ShaderResourceView> pSRVIrradSH;
	//	CD3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc(D3D11_SRV_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, 0, (UINT)irradianceBufferElements);
	//	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(cbIrradianceSH.Get(), &SRVDesc, pSRVIrradSH.GetAddressOf()));

	//	std::string UAVDebugName = "SHIrrad Buffer UAV" + std::to_string(passId);
	//	pUAVIrradSH->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)UAVDebugName.size(), UAVDebugName.c_str());

	//	std::string SRVDebugName = "SHIrrad Buffer SRV" + std::to_string(passId);
	//	pSRVIrradSH->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)SRVDebugName.size(), SRVDebugName.c_str());
	//	pSRVIrradSHs.push_back(pSRVIrradSH);
	//	if (passId == SHIrradPassCount - 1)
	//		m_pSRVIrradianceSH = pSRVIrradSH;

	//	g_pContext->CSSetUnorderedAccessViews(0, 1, pUAVIrradSH.GetAddressOf(), nullptr);

	//	if (passId == 0)
	//	{
	//		auto pSRV = pTexHDR->GetSRV();
	//		g_pContext->CSSetShaderResources(0, 1, &pSRV);
	//	}
	//	else
	//	{
	//		g_pContext->CSSetShaderResources(0, 1, pSRVIrradSHs[passId - 1].GetAddressOf());
	//	}

	//	g_pContext->CSSetShader(pComputeShader.Get(), nullptr, 0);

	//	g_pContext->Dispatch(tempWidth, tempHeight, 1);

	//	// 用完以后清空对应槽位的SRV UAV，避免后续pass资源绑不上（可能有优化空间）
	//	ComPtr<ID3D11ShaderResourceView> pSRVNull[1] = { nullptr };
	//	g_pContext->CSSetShaderResources(0, 1, pSRVNull->GetAddressOf());

	//	ComPtr<ID3D11UnorderedAccessView> pUAVNull[1] = { nullptr };
	//	g_pContext->CSSetUnorderedAccessViews(0, 1, pUAVNull->GetAddressOf(), nullptr);

	//	g_pUDA->EndEvent();
	//}

	//g_pUDA->EndEvent();
}
