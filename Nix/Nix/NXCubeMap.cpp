#include "NXCubeMap.h"
#include "SphereHarmonics.h"
#include "NXGlobalDefinitions.h"
#include "NXScene.h"
#include "NXCamera.h"
#include "ShaderComplier.h"
#include "DirectResources.h"
#include "NXRenderStates.h"
#include "NXSamplerManager.h"
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
}

bool NXCubeMap::Init(const std::filesystem::path& filePath)
{
	m_pCommandQueue = NX12Util::CreateCommandQueue(NXGlobalDX::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT, true);
	m_pCommandQueue->SetName(L"CubeMap Building Command Queue");

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
		GenerateIrradianceSHFromCubeMap();
		GeneratePreFilterMap();
	}
	else if (strExtension == ".hdr")
	{
		Ntr<NXTexture2D> pTexHDR = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("HDR Temp Texture", filePath);
		GenerateCubeMap(pTexHDR, [&](Ntr<NXTextureCube> pTexCubeMap) {
			m_pTexCubeMap = pTexCubeMap;
			GeneratePreFilterMap();
			});
	}

	return true;
}

const D3D12_CPU_DESCRIPTOR_HANDLE& NXCubeMap::GetSRVCubeMap()
{
	return m_pTexCubeMap->GetSRV();
}

const D3D12_CPU_DESCRIPTOR_HANDLE& NXCubeMap::GetSRVCubeMapPreview2D()
{
	return m_pTexCubeMap->GetSRVPreview2D();
}

const D3D12_CPU_DESCRIPTOR_HANDLE& NXCubeMap::GetSRVIrradianceMap()
{
	return m_pTexIrradianceMap->GetSRV();
}

const D3D12_CPU_DESCRIPTOR_HANDLE& NXCubeMap::GetSRVPreFilterMap()
{
	return m_pTexPreFilterMap->GetSRV();
}

void NXCubeMap::Update()
{
	auto pCamera = m_pScene->GetMainCamera();
	m_cbDataCubeWVPMatrix.world = Matrix::CreateTranslation(pCamera->GetTranslation()).Transpose();
	m_cbDataCubeWVPMatrix.view = pCamera->GetViewMatrix().Transpose();
	m_cbDataCubeWVPMatrix.projection = pCamera->GetProjectionMatrix().Transpose();
	m_cbCubeWVPMatrix.Set(m_cbDataCubeWVPMatrix);
}

void NXCubeMap::Release()
{
	NXTransform::Release();
}

void NXCubeMap::GenerateCubeMap(Ntr<NXTexture2D>& pTexHDR, GenerateCubeMapCallback pCubeMapCallBack)
{
	const static float mapSize = 1024;
	auto vp = NX12Util::ViewPort(mapSize, mapSize);

	Ntr<NXTextureCube> pTexCubeMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTextureCube("Main CubeMap", DXGI_FORMAT_R32G32B32A32_FLOAT, (UINT)mapSize, (UINT)mapSize, 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, false);
	pTexCubeMap->SetViews(1, NXCUBEMAP_FACE_COUNT, 0, 0);
	pTexCubeMap->SetSRV(0);
	for (int i = 0; i < NXCUBEMAP_FACE_COUNT; i++)
		pTexCubeMap->SetRTV(i, 0, i, 1);

	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\HDRToCubeMap.fx", "VS", pVSBlob.GetAddressOf());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\HDRToCubeMap.fx", "PS", pPSBlob.GetAddressOf());

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSigCubeMap.Get();
	psoDesc.InputLayout = NXGlobalInputLayout::layoutP;
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<false>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = pTexCubeMap->GetFormat();
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSOCubeMap));

	ConstantBufferBaseWVP cbWVP;
	cbWVP.world = Matrix::Identity();
	cbWVP.projection = m_mxCubeMapProj.Transpose();

	std::thread([&]() {
		for (int i = 0; i < NXCUBEMAP_FACE_COUNT; i++)
		{
			NXConstantBuffer<ConstantBufferBaseWVP> cbCubeWVP;
			cbWVP.view = m_mxCubeMapView[i].Transpose();
			cbCubeWVP.Set(cbWVP);

			ComPtr<ID3D12CommandAllocator> pCmdAllocator;
			ComPtr<ID3D12GraphicsCommandList> pCmdList;

			NX12Util::CreateCommands(NXGlobalDX::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT, pCmdAllocator.GetAddressOf(), pCmdList.GetAddressOf());
			pCmdList->Reset(pCmdAllocator.Get(), nullptr);

			std::string str = "Generate Cube Map " + std::to_string(i);
			NX12Util::BeginEvent(pCmdList.Get(), str.c_str());

			pCmdList->RSSetViewports(1, &vp);
			pCmdList->RSSetScissorRects(1, &NX12Util::ScissorRect(vp));

			pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			ID3D12DescriptorHeap* ppHeaps[] = { NXShVisDescHeap->GetDescriptorHeap() };
			pCmdList->SetDescriptorHeaps(1, ppHeaps);

			pCmdList->SetGraphicsRootSignature(m_pRootSigCubeMap.Get());
			pCmdList->SetPipelineState(m_pPSOCubeMap.Get());

			pTexCubeMap->SetResourceState(pCmdList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = pTexCubeMap->GetRTV(i);
			pCmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

			NXShVisDescHeap->PushFluid(pTexHDR->GetSRV(0));
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandles = NXShVisDescHeap->Submit();

			pCmdList->SetGraphicsRootConstantBufferView(0, cbCubeWVP.CurrentGPUAddress());
			pCmdList->SetGraphicsRootDescriptorTable(1, gpuHandles);

			const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews("_CubeMapBox");
			pCmdList->IASetVertexBuffers(0, 1, &meshView.vbv);
			pCmdList->IASetIndexBuffer(&meshView.ibv);
			pCmdList->DrawIndexedInstanced(6, 1, i * 6, 0, 0);

			NX12Util::EndEvent(pCmdList.Get());

			pCmdList->Close();
			ID3D12CommandList* ppCmdList[] = { pCmdList.Get() };
			m_pCommandQueue->ExecuteCommandLists(1, ppCmdList);
		}

		// 等待渲染完再CallBack，不然 pTexCubeMap 的数据还没写入
		HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		m_nFenceValue++;
		m_pCommandQueue->Signal(m_pFence.Get(), m_nFenceValue);

		if (m_pFence->GetCompletedValue() < m_nFenceValue)
		{
			m_pFence->SetEventOnCompletion(m_nFenceValue, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
		}

		if (pCubeMapCallBack)
			pCubeMapCallBack(pTexCubeMap);

		}).detach();
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
	// 读取CubeMap
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

	for (int i = 0; i < MultiFrameSets_swapChainCount; i++)
	{
		ConstantBufferCubeMap cbData = m_cbDataCubeMap;
		cbData.irradSH0123x = Vector4(m_shIrradianceMap[0].x, m_shIrradianceMap[1].x, m_shIrradianceMap[2].x, m_shIrradianceMap[3].x);
		cbData.irradSH0123y = Vector4(m_shIrradianceMap[0].y, m_shIrradianceMap[1].y, m_shIrradianceMap[2].y, m_shIrradianceMap[3].y);
		cbData.irradSH0123z = Vector4(m_shIrradianceMap[0].z, m_shIrradianceMap[1].z, m_shIrradianceMap[2].z, m_shIrradianceMap[3].z);
		cbData.irradSH4567x = Vector4(m_shIrradianceMap[4].x, m_shIrradianceMap[5].x, m_shIrradianceMap[6].x, m_shIrradianceMap[7].x);
		cbData.irradSH4567y = Vector4(m_shIrradianceMap[4].y, m_shIrradianceMap[5].y, m_shIrradianceMap[6].y, m_shIrradianceMap[7].y);
		cbData.irradSH4567z = Vector4(m_shIrradianceMap[4].z, m_shIrradianceMap[5].z, m_shIrradianceMap[6].z, m_shIrradianceMap[7].z);
		cbData.irradSH8xyz = m_shIrradianceMap[8];
		m_cbCubeMap.Set(m_cbDataCubeMap);
	}
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
	//NX::ThrowIfFailed(NXGlobalDX::GetDevice()->CreateBuffer(&bufferDesc, nullptr, &cb));

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
	//	g_pContext->ClearRenderTargetView(pRTV, Colors::Black);
	//	g_pContext->OMSetRenderTargets(1, &pRTV, nullptr);

	//	cbData.view = m_mxCubeMapView[i].Transpose();
	//	g_pContext->UpdateSubresource(cb.Get(), 0, nullptr, &cbData, 0, 0);
	//	g_pContext->DrawIndexed((UINT)m_indicesCubeBox.size() / 6, i * 6, 0);
	//}

	//g_pUDA->EndEvent();
}

void NXCubeMap::GeneratePreFilterMap()
{
	float mapSize = 512.0f;
	m_pTexPreFilterMap = NXResourceManager::GetInstance()->GetTextureManager()->CreateTextureCube("PreFilter Map", m_pTexCubeMap->GetFormat(), (UINT)mapSize, (UINT)mapSize, NXROUGHNESS_FILTER_COUNT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, false);
	m_pTexPreFilterMap->SetViews(1, NXCUBEMAP_FACE_COUNT * NXROUGHNESS_FILTER_COUNT, 0, 0);
	m_pTexPreFilterMap->SetSRV(0);

	for (int i = 0; i < NXROUGHNESS_FILTER_COUNT; i++) // 5
	{
		for (int j = 0; j < NXCUBEMAP_FACE_COUNT; j++) // 6
		{
			int index = i * NXCUBEMAP_FACE_COUNT + j;
			// mipSize = i, firstarray = j, arraysize = 1
			m_pTexPreFilterMap->SetRTV(index, i, j, 1);
		}
	}

	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	NXShaderComplier::GetInstance()->CompileVS(L"Shader\\CubeMapPreFilter.fx", "VS", pVSBlob.GetAddressOf());
	NXShaderComplier::GetInstance()->CompilePS(L"Shader\\CubeMapPreFilter.fx", "PS", pPSBlob.GetAddressOf());

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_pRootSigPreFilterMap.Get();
	psoDesc.InputLayout = NXGlobalInputLayout::layoutPNT;
	psoDesc.BlendState = NXBlendState<>::Create();
	psoDesc.RasterizerState = NXRasterizerState<>::Create();
	psoDesc.DepthStencilState = NXDepthStencilState<false>::Create();
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = m_pTexPreFilterMap->GetFormat();
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	psoDesc.VS = { pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize() };
	psoDesc.PS = { pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize() };
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	NXGlobalDX::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pPSOPreFilterMap));


	NXConstantBuffer<ConstantBufferObject> cbCubeCamera[NXCUBEMAP_FACE_COUNT];
	for (int i = 0; i < NXCUBEMAP_FACE_COUNT; i++)
	{
		ConstantBufferObject cbData;
		cbData.world = Matrix::Identity();
		cbData.projection = m_mxCubeMapProj.Transpose();
		cbData.view = m_mxCubeMapView[i].Transpose();
		cbCubeCamera[i].Set(cbData);
	}

	float rough[NXROUGHNESS_FILTER_COUNT] = { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f };

	NXConstantBuffer<ConstantBufferFloat> cbRoughness[NXROUGHNESS_FILTER_COUNT];
	for (int i = 0; i < NXROUGHNESS_FILTER_COUNT; i++)
	{
		ConstantBufferFloat cbData;
		cbData.value = rough[i] * rough[i];
		cbRoughness[i].Set(cbData);
	}

	std::thread([&]() {
		for (int i = 0; i < NXROUGHNESS_FILTER_COUNT; i++)
		{
			for (int j = 0; j < NXCUBEMAP_FACE_COUNT; j++)
			{
				ComPtr<ID3D12CommandAllocator> pCmdAllocator;
				ComPtr<ID3D12GraphicsCommandList> pCmdList;
				NX12Util::CreateCommands(NXGlobalDX::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT, pCmdAllocator.GetAddressOf(), pCmdList.GetAddressOf());
				pCmdList->Reset(pCmdAllocator.Get(), nullptr);

				NX12Util::BeginEvent(pCmdList.Get(), "Generate PreFilter Map");

				pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

				ID3D12DescriptorHeap* ppHeaps[] = { NXShVisDescHeap->GetDescriptorHeap() };
				pCmdList->SetDescriptorHeaps(1, ppHeaps);

				pCmdList->SetGraphicsRootSignature(m_pRootSigPreFilterMap.Get());
				pCmdList->SetPipelineState(m_pPSOPreFilterMap.Get());

				m_pTexPreFilterMap->SetResourceState(pCmdList.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET);

				//////

				float mipSize = (float)((UINT)mapSize >> i);

				auto vp = NX12Util::ViewPort(mipSize, mipSize);
				pCmdList->RSSetViewports(1, &vp);
				pCmdList->RSSetScissorRects(1, &NX12Util::ScissorRect(vp));

				D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pTexPreFilterMap->GetRTV(i * 6 + j);
				pCmdList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

				NXShVisDescHeap->PushFluid(m_pTexCubeMap->GetSRV(0));
				D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = NXShVisDescHeap->Submit();

				pCmdList->SetGraphicsRootConstantBufferView(0, cbCubeCamera[j].CurrentGPUAddress());
				pCmdList->SetGraphicsRootConstantBufferView(1, cbRoughness[i].CurrentGPUAddress());
				pCmdList->SetGraphicsRootDescriptorTable(2, gpuHandle);

				const NXMeshViews& meshView = NXSubMeshGeometryEditor::GetInstance()->GetMeshViews("_CubeMapBox");
				pCmdList->IASetVertexBuffers(0, 1, &meshView.vbv);
				pCmdList->IASetIndexBuffer(&meshView.ibv);
				pCmdList->DrawIndexedInstanced(6, 1, j * 6, 0, 0);

				NX12Util::EndEvent(pCmdList.Get());

				pCmdList->Close();
				ID3D12CommandList* ppCmdList[] = { pCmdList.Get() };
				m_pCommandQueue->ExecuteCommandLists(1, ppCmdList);
			}
		}
		}).detach();
}

void NXCubeMap::SetIntensity(float val)
{
	m_cbDataCubeMap.intensity = val;
	m_cbCubeMap.Set(m_cbDataCubeMap);
}

void NXCubeMap::SetIrradMode(int val)
{
	m_cbDataCubeMap.irradMode = Vector4((float)val);
	m_cbCubeMap.Set(m_cbDataCubeMap);
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

	NXSubMeshGeometryEditor::GetInstance()->CreateVBIB(std::move(m_vertices), std::move(m_indices), "_CubeMapSphere");
	NXSubMeshGeometryEditor::GetInstance()->CreateVBIB(std::move(m_verticesCubeBox), std::move(m_indicesCubeBox), "_CubeMapBox");
}

void NXCubeMap::InitRootSignature()
{
	std::vector<D3D12_STATIC_SAMPLER_DESC> pSamplers = {
		NXSamplerManager::GetInstance()->CreateIso(0, 0, D3D12_SHADER_VISIBILITY_ALL) // s0
	};

	// cubemap
	std::vector<D3D12_DESCRIPTOR_RANGE> rangesCubeMap = {
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0) // t0 ~ t0.
	};

	std::vector<D3D12_ROOT_PARAMETER> rootParamsCubeMap = {
		NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL), // b0
		NX12Util::CreateRootParameterTable(rangesCubeMap, D3D12_SHADER_VISIBILITY_ALL) // 上面的 rangesCubeMap. t0 ~ t0.
	};

	m_pRootSigCubeMap = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), rootParamsCubeMap, pSamplers);

	// prefilter map
	std::vector<D3D12_DESCRIPTOR_RANGE> rangesPreFilter = {
		NX12Util::CreateDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0) // t0 ~ t0.
	};

	std::vector<D3D12_ROOT_PARAMETER> rootParamsPreFilter = {
		NX12Util::CreateRootParameterCBV(0, 0, D3D12_SHADER_VISIBILITY_ALL), // b0
		NX12Util::CreateRootParameterCBV(1, 0, D3D12_SHADER_VISIBILITY_ALL), // b1
		NX12Util::CreateRootParameterTable(rangesPreFilter, D3D12_SHADER_VISIBILITY_ALL) // 上面的 rangesPreFilter. t0 ~ t0.
	};

	m_pRootSigPreFilterMap = NX12Util::CreateRootSignature(NXGlobalDX::GetDevice(), rootParamsPreFilter, pSamplers);
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
	//NX::ThrowIfFailed(NXGlobalDX::GetDevice()->CreateBuffer(&bufferDesc, nullptr, &cbImageSize));

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
	//	NX::ThrowIfFailed(NXGlobalDX::GetDevice()->CreateBuffer(&bufferDesc, nullptr, &cbIrradianceSH));

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
	//	NX::ThrowIfFailed(NXGlobalDX::GetDevice()->CreateUnorderedAccessView(cbIrradianceSH.Get(), &UAVDesc, pUAVIrradSH.GetAddressOf()));

	//	ComPtr<ID3D11ShaderResourceView> pSRVIrradSH;
	//	CD3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc(D3D11_SRV_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN, 0, (UINT)irradianceBufferElements);
	//	NX::ThrowIfFailed(NXGlobalDX::GetDevice()->CreateShaderResourceView(cbIrradianceSH.Get(), &SRVDesc, pSRVIrradSH.GetAddressOf()));

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
