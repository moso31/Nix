#include "NXTexture.h"
#include "NXGlobalDefinitions.h"
#include "DirectXTex.h"
#include "NXResourceManager.h"
#include "NXConverter.h"
#include "NXLog.h"
#include "NXRandom.h"
#include "NXAllocatorManager.h"

ComPtr<ID3D12CommandAllocator> NXTexture::s_pCmdAllocator = nullptr;
ComPtr<ID3D12GraphicsCommandList> NXTexture::s_pCmdList = nullptr;

NXTexture::~NXTexture()
{
	//NXLog::LogWithStackTrace("[%s : size=(%dx%d)x%d, mip=%d, path=%s] Deleted. remain RefCount: %d", m_name.c_str(), m_width, m_height, m_arraySize, m_mipLevels, m_texFilePath.string().c_str(), m_nRefCount);
}

void NXTexture::Init()
{
	s_pCmdAllocator = NX12Util::CreateCommandAllocator(NXGlobalDX::GetDevice(), D3D12_COMMAND_LIST_TYPE_DIRECT);
	s_pCmdList = NX12Util::CreateGraphicsCommandList(NXGlobalDX::GetDevice(), s_pCmdAllocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT);
}

const XShaderDescriptor& NXTexture::GetSRV(uint32_t index) 
{
	WaitLoadingViewsFinish();
	return m_pSRVs[index];
}

const XDescriptor& NXTexture::GetRTV(uint32_t index)
{
	WaitLoadingViewsFinish();
	return m_pRTVs[index];
}

const XDescriptor& NXTexture::GetDSV(uint32_t index)
{
	WaitLoadingViewsFinish();
	return m_pDSVs[index];
}

const XShaderDescriptor& NXTexture::GetUAV(uint32_t index)
{
	WaitLoadingViewsFinish();
	return m_pUAVs[index];
}

void NXTexture::SetViews(uint32_t srvNum, uint32_t rtvNum, uint32_t dsvNum, uint32_t uavNum, bool bAutoSubmitViews)
{
	m_pSRVs.resize(srvNum);
	m_pRTVs.resize(rtvNum);
	m_pDSVs.resize(dsvNum);
	m_pUAVs.resize(uavNum);

	if (bAutoSubmitViews)
	{
		SubmitLoadingViews(srvNum + rtvNum + dsvNum + uavNum);
	}
}

void NXTexture::SubmitLoadingViews(int asyncLoadingViewsCount)
{
	m_loadingViews = asyncLoadingViewsCount;
}

void NXTexture::ProcessLoadingBuffers()
{
	m_loadingViews.fetch_add(-1);

	if (m_loadingViews == 0)
	{
		m_promiseLoadingViews.set_value();
	}
}

void NXTexture::WaitLoadingViewsFinish()
{
	m_futureLoadingViews.wait();
}

void NXTexture::SetClearValue(float R, float G, float B, float A)
{
	m_clearValue.Color[0] = R;
	m_clearValue.Color[1] = G;
	m_clearValue.Color[2] = B;
	m_clearValue.Color[3] = A;
	m_clearValue.Format = NXConvert::DXGINoTypeless(m_texFormat);
}

void NXTexture::SetClearValue(float depth, uint32_t stencilRef)
{
	m_clearValue.DepthStencil.Depth = depth;
	m_clearValue.DepthStencil.Stencil = stencilRef;
	m_clearValue.Format = NXConvert::DXGINoTypeless(m_texFormat, true);
}

const void NXTexture::SetResourceState(ID3D12GraphicsCommandList* pCommandList, const D3D12_RESOURCE_STATES& state)
{
	if (m_resourceState == state)
		return;

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_pTexture.Get();
	barrier.Transition.StateBefore = m_resourceState;
	barrier.Transition.StateAfter = state;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pCommandList->ResourceBarrier(1, &barrier);

	m_resourceState = state;
}

void NXTexture::SwapToReloadingTexture()
{
	if (m_reloadingState == NXTextureReloadingState::Texture_StartReload)
	{
		InternalReload(NXResourceManager::GetInstance()->GetTextureManager()->GetCommonTextures(NXCommonTex_White));
	}

	if (m_reloadingState == NXTextureReloadingState::Texture_FinishReload)
	{
		InternalReload(m_pReloadingTexture);
		m_pReloadingTexture = nullptr;
	}
}

void NXTexture::CreateRenderTextureInternal(D3D12_RESOURCE_FLAGS flags)
{
	// 创建RT纹理时，没有从文件读取资源的需求，不需要提供上传堆资源。
	// 不走任何Allocator，直接创建资源；ComPtr自己管理资源m_pTexture的生命周期
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = GetResourceDimentionFromType();
	desc.Width = m_width;
	desc.Height = m_height;
	desc.DepthOrArraySize = m_arraySize;
	desc.MipLevels = m_mipLevels;
	desc.Format = m_texFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = flags;

	HRESULT hr;
	if (flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
	{
		m_resourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		SetClearValue(0.0f, 0.0f, 0.0f, 1.0f);

	}
	else if (flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		m_resourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		SetClearValue(1.0f, 0x00);
	}
	else
	{
		m_resourceState = D3D12_RESOURCE_STATE_COMMON;
		SetClearValue(1.0f, 0x00);
	}

	hr = NXGlobalDX::GetDevice()->CreateCommittedResource(
		&NX12Util::CreateHeapProperties(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		m_resourceState,
		&m_clearValue,
		IID_PPV_ARGS(&m_pTexture)
	);

	std::wstring wName(m_name.begin(), m_name.end());
	m_pTexture->SetName(wName.c_str());
	SetRefCountDebugName(m_name);

	if (FAILED(hr))
	{
		m_pTexture.Reset();
		return;
	}
}

void NXTexture::CreateInternal(std::unique_ptr<DirectX::ScratchImage>&& pImage, D3D12_RESOURCE_FLAGS flags)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = GetResourceDimentionFromType();
	desc.Width = m_width;
	desc.Height = m_height;
	desc.DepthOrArraySize = m_arraySize;
	desc.MipLevels = m_mipLevels;
	desc.Format = m_texFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = flags;

	uint32_t layoutSize = desc.DepthOrArraySize * desc.MipLevels;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts = new D3D12_PLACED_SUBRESOURCE_FOOTPRINT[layoutSize];
	uint32_t* numRow = new uint32_t[layoutSize];
	UINT64* numRowSizeInBytes = new UINT64[layoutSize];
	size_t totalBytes;
	NXGlobalDX::GetDevice()->GetCopyableFootprints(&desc, 0, layoutSize, 0, layouts, numRow, numRowSizeInBytes, &totalBytes);

	NXAllocator_Tex->Alloc(&desc, totalBytes, [this, layouts, numRow, numRowSizeInBytes, totalBytes, pImage = std::move(pImage)](const PlacedBufferAllocTaskResult& result) mutable {
		m_pTexture = result.pResource;

		UploadTaskContext taskContext;
		if (NXUploadSystem->BuildTask((int)totalBytes, taskContext))
		{
			// 更新纹理资源
			m_pTexture->SetName(NXConvert::s2ws(m_name).c_str());
			SetRefCountDebugName(m_name);
			m_resourceState = D3D12_RESOURCE_STATE_COPY_DEST; // 和 NXAllocator_Tex->Alloc 内部的逻辑保持同步

			auto texDesc = m_pTexture->GetDesc();
			for (uint32_t face = 0, index = 0; face < texDesc.DepthOrArraySize; face++)
			{
				for (uint32_t mip = 0; mip < texDesc.MipLevels; mip++, index++)
				{
					const Image* pImg = pImage->GetImage(mip, face, 0);
					const BYTE* pSrcData = pImg->pixels;
					BYTE* pMappedRingBufferData = taskContext.pResourceData + taskContext.pResourceOffset;
					BYTE* pDstData = pMappedRingBufferData + layouts[index].Offset;

					for (uint32_t y = 0; y < numRow[index]; y++)
					{
						memcpy(pDstData + layouts[index].Footprint.RowPitch * y, pSrcData + pImg->rowPitch * y, numRowSizeInBytes[index]);
					}
				}
			}

			NXUploadSystem->FinishTask(taskContext);

			delete[] layouts;
			delete[] numRow;
			delete[] numRowSizeInBytes;

			pImage.reset();
		}
	}); 
}

void NXTexture::CreatePathTextureInternal(const std::filesystem::path& filePath, D3D12_RESOURCE_FLAGS flags)
{
	// 主线程加载一次metadata，需要获取基本的信息，不然后面纹理没法算
	TexMetadata metadata;
	GetMetadataFromFile(filePath, metadata);
	m_width = (uint32_t)metadata.width;
	m_height = (uint32_t)metadata.height;
	m_arraySize = (uint32_t)metadata.arraySize;
	m_mipLevels = (uint32_t)metadata.mipLevels;
	m_texFormat = metadata.format;

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = GetResourceDimentionFromType();
	desc.Width = m_width;
	desc.Height = m_height;
	desc.DepthOrArraySize = m_arraySize;
	desc.MipLevels = m_mipLevels;
	desc.Format = m_texFormat;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = flags;

	uint32_t layoutSize = desc.DepthOrArraySize * desc.MipLevels;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* layouts = new D3D12_PLACED_SUBRESOURCE_FOOTPRINT[layoutSize];
	uint32_t* numRow = new uint32_t[layoutSize];
	UINT64* numRowSizeInBytes = new UINT64[layoutSize];
	size_t totalBytes;
	NXGlobalDX::GetDevice()->GetCopyableFootprints(&desc, 0, layoutSize, 0, layouts, numRow, numRowSizeInBytes, &totalBytes);

	NXAllocator_Tex->Alloc(&desc, totalBytes, [this, filePath, layouts, numRow, numRowSizeInBytes, totalBytes](const PlacedBufferAllocTaskResult& result) {
		m_pTexture = result.pResource;

		UploadTaskContext taskContext;
		if (NXUploadSystem->BuildTask((int)totalBytes, taskContext))
		{
			// TODO: 异步 // 不用了 其实已经是异步了……
			{
				TexMetadata metadata;
				std::unique_ptr<ScratchImage> pImage = std::make_unique<ScratchImage>();

				HRESULT hr;
				std::string strExtension = NXConvert::s2lower(filePath.extension().string());
				if (strExtension == ".hdr")
					hr = LoadFromHDRFile(filePath.c_str(), &metadata, *pImage);
				else if (strExtension == ".dds")
					hr = LoadFromDDSFile(filePath.c_str(), DDS_FLAGS_NONE, &metadata, *pImage);
				else if (strExtension == ".tga")
					hr = LoadFromTGAFile(filePath.c_str(), &metadata, *pImage);
				else
					hr = LoadFromWICFile(filePath.c_str(), WIC_FLAGS_NONE, &metadata, *pImage);

				if (FAILED(hr))
				{
					std::wstring errMsg = L"Failed to load texture file." + filePath.wstring();
					MessageBox(NULL, errMsg.c_str(), L"Error", MB_OK | MB_ICONERROR);
					pImage.reset();
					return;
				}

				// 如果是Texture2D纹理，并且读取的是arraySize/TextureCube 类型的文件，就只加载第一面。
				if (metadata.arraySize > 1 && m_type == TextureType_2D)
				{
					std::unique_ptr<ScratchImage> timage(new ScratchImage);
					timage->InitializeFromImage(*pImage->GetImage(0, 0, 0));
					metadata = timage->GetMetadata();
					pImage.swap(timage);
				}

				if (NXConvert::IsUnormFormat(metadata.format))
				{
					DXGI_FORMAT safeFormat = NXConvert::SafeDXGIFormat(metadata.format);
					if (metadata.format != safeFormat)
					{
						std::unique_ptr<ScratchImage> timage(new ScratchImage);
						hr = Convert(pImage->GetImages(), pImage->GetImageCount(), pImage->GetMetadata(), safeFormat, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, *timage);
						if (SUCCEEDED(hr))
						{
							metadata.format = safeFormat;
						}
						else
						{
							printf("Warning: [Convert] failed when loading NXTexture2D: %s.\n", m_texFilePath.string().c_str());
						}
						pImage.swap(timage);
					}
				}

				// 如果序列化的文件里记录了sRGB/Linear类型，就做对应的转换
				if (m_serializationData.m_textureType == NXTextureMode::sRGB || m_serializationData.m_textureType == NXTextureMode::Linear)
				{
					bool bIsSRGB = m_serializationData.m_textureType == NXTextureMode::sRGB;
					DXGI_FORMAT tFormat = bIsSRGB ? NXConvert::ForceSRGB(metadata.format) : NXConvert::ForceLinear(metadata.format);
					if (metadata.format != tFormat)
					{
						std::unique_ptr<ScratchImage> timage(new ScratchImage);

						TEX_FILTER_FLAGS texFlags = bIsSRGB ? TEX_FILTER_SRGB_IN : TEX_FILTER_DEFAULT;
						hr = Convert(pImage->GetImages(), pImage->GetImageCount(), pImage->GetMetadata(), tFormat, texFlags, TEX_THRESHOLD_DEFAULT, *timage);
						if (SUCCEEDED(hr))
						{
							metadata.format = tFormat;
						}
						else
						{
							printf("Warning: [Convert] failed when loading NXTexture2D: %s\n", m_texFilePath.string().c_str());
						}
						pImage.swap(timage);
					}
				}

				// --- Invert Y Channel --------------------------------------------------------
				if (m_serializationData.m_bInvertNormalY)
				{
					std::unique_ptr<ScratchImage> timage(new ScratchImage);

					HRESULT hr = TransformImage(pImage->GetImages(), pImage->GetImageCount(), pImage->GetMetadata(),
						[&](XMVECTOR* outPixels, const XMVECTOR* inPixels, size_t w, size_t y)
						{
							static const XMVECTORU32 s_selecty = { { { XM_SELECT_0, XM_SELECT_1, XM_SELECT_0, XM_SELECT_0 } } };
							UNREFERENCED_PARAMETER(y);

							for (size_t j = 0; j < w; ++j)
							{
								const XMVECTOR value = inPixels[j];
								const XMVECTOR inverty = XMVectorSubtract(g_XMOne, value);
								outPixels[j] = XMVectorSelect(value, inverty, s_selecty);
							}
						}, *timage);

					if (FAILED(hr))
					{
						printf("Warning: [InvertNormalY] failed when loading NXTexture2D: %s\n", m_texFilePath.string().c_str());
					}

					pImage.swap(timage);
				}

				if (m_serializationData.m_bGenerateMipMap && metadata.width >= 2 && metadata.height >= 2 && metadata.mipLevels == 1)
				{
					std::unique_ptr<ScratchImage> pImageMip = std::make_unique<ScratchImage>();
					HRESULT hr = GenerateMipMaps(pImage->GetImages(), pImage->GetImageCount(), pImage->GetMetadata(), TEX_FILTER_DEFAULT, 0, *pImageMip);
					if (SUCCEEDED(hr))
					{
						metadata.mipLevels = pImageMip->GetMetadata().mipLevels;
						pImage.swap(pImageMip);
					}
					else
					{
						printf("Warning: [GenerateMipMap] failed when loading NXTexture2D: %s\n", m_texFilePath.string().c_str());
					}
				}


				// 更新纹理资源
				m_pTexture->SetName(NXConvert::s2ws(m_name).c_str());
				SetRefCountDebugName(m_name);
				m_resourceState = D3D12_RESOURCE_STATE_COPY_DEST; // 和 NXAllocator_Tex->Alloc 内部的逻辑保持同步

				auto texDesc = m_pTexture->GetDesc();
				for (uint32_t face = 0, index = 0; face < texDesc.DepthOrArraySize; face++)
				{
					for (uint32_t mip = 0; mip < texDesc.MipLevels; mip++, index++)
					{
						const Image* pImg = pImage->GetImage(mip, face, 0);
						const BYTE* pSrcData = pImg->pixels;
						BYTE* pMappedRingBufferData = taskContext.pResourceData + taskContext.pResourceOffset;
						BYTE* pDstData = pMappedRingBufferData + layouts[index].Offset;

						for (uint32_t y = 0; y < numRow[index]; y++)
						{
							memcpy(pDstData + layouts[index].Footprint.RowPitch * y, pSrcData + pImg->rowPitch * y, numRowSizeInBytes[index]);
						}
					}
				}

				NXUploadSystem->FinishTask(taskContext);

				delete[] layouts;
				delete[] numRow;
				delete[] numRowSizeInBytes;

				pImage.reset();
			}
		}
		else
		{
			// 抛出异常
			printf("Error: [NXUploadSystem::BuildTask] failed when loading NXTexture2D: %s\n", m_texFilePath.string().c_str());
		}
	});
}

bool NXTexture::GetMetadataFromFile(const std::filesystem::path& path, TexMetadata& oMetaData)
{
	HRESULT hr;
	TexMetadata metadata;
	std::string strExtension = NXConvert::s2lower(path.extension().string());
	if (strExtension == ".hdr")
		hr = GetMetadataFromHDRFile(path.wstring().c_str(), metadata);
	else if (strExtension == ".dds")
		hr = GetMetadataFromDDSFile(path.wstring().c_str(), DDS_FLAGS_NONE, metadata);
	else if (strExtension == ".tga")
		hr = GetMetadataFromTGAFile(path.wstring().c_str(), metadata);
	else
		hr = GetMetadataFromWICFile(path.wstring().c_str(), WIC_FLAGS_NONE, metadata);

	return SUCCEEDED(hr);
}

void NXTexture::InternalReload(Ntr<NXTexture> pReloadTexture)
{
	m_pTexture = pReloadTexture->m_pTexture;

	auto& pReloadSRVs = pReloadTexture->m_pSRVs;
	m_pSRVs.resize(pReloadSRVs.size());

	auto& pReloadDSVs = pReloadTexture->m_pDSVs;
	m_pDSVs.resize(pReloadDSVs.size());

	auto& pReloadRTVs = pReloadTexture->m_pRTVs;
	m_pRTVs.resize(pReloadRTVs.size());

	auto& pReloadUAVs = pReloadTexture->m_pUAVs;
	m_pUAVs.resize(pReloadUAVs.size());

	for (size_t i = 0; i < pReloadSRVs.size(); i++) m_pSRVs[i] = pReloadSRVs[i];
	for (size_t i = 0; i < pReloadDSVs.size(); i++) m_pDSVs[i] = pReloadDSVs[i];
	for (size_t i = 0; i < pReloadRTVs.size(); i++) m_pRTVs[i] = pReloadRTVs[i];
	for (size_t i = 0; i < pReloadUAVs.size(); i++) m_pUAVs[i] = pReloadUAVs[i];
}

D3D12_RESOURCE_DIMENSION NXTexture::GetResourceDimentionFromType()
{
	switch (m_type)
	{
	case TextureType_None: 
		return D3D12_RESOURCE_DIMENSION_UNKNOWN;
	case TextureType_1D: 
		return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	case TextureType_2D: 
	case TextureType_Cube:
	case TextureType_2DArray:
		return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	case TextureType_3D:
		return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	default:
		return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	}
}

NXTextureReloadTask NXTexture::LoadTextureAsync()
{
	co_await NXTextureAwaiter();

	// 从这里开始异步加载...
	LoadTextureSync();
}

void NXTexture::LoadTextureSync()
{
	auto pOldTex = m_pReloadingTexture;
	m_pReloadingTexture = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D(m_name, m_texFilePath, true); 
}

void NXTexture::Release()
{
}

void NXTexture::MarkReload()
{
	if (m_reloadingState == NXTextureReloadingState::Texture_None)
	{
		m_reloadingState = NXTextureReloadingState::Texture_StartReload;
		return;
	}
}

void NXTexture::OnReloadFinish()
{
	m_reloadingState = NXTextureReloadingState::Texture_FinishReload;
}

void NXTexture::Serialize()
{
	using namespace rapidjson;

	if (m_texFilePath.empty())
	{
		printf("Warning, %s couldn't be serialized, cause path %s does not exist.\n", m_texFilePath.string().c_str(), m_texFilePath.string().c_str());
		return;
	}

	std::string nxInfoPath = m_texFilePath.string() + ".n0";

	// 2023.5.30 纹理资源的序列化: 
	NXSerializer serializer;
	serializer.StartObject();
	serializer.String("NXInfoPath", nxInfoPath);	// 元文件路径
	serializer.Uint64("PathHashValue", std::filesystem::hash_value(m_texFilePath)); // 纹理文件路径 hash value
	serializer.Int("TextureType", (int)m_serializationData.m_textureType); // 纹理类型
	serializer.Bool("IsInvertNormalY", m_serializationData.m_bInvertNormalY); // 是否FlipY法线
	serializer.Bool("IsGenerateMipMap", m_serializationData.m_bGenerateMipMap); // 是否生成mipmap
	serializer.Bool("IsCubeMap", m_serializationData.m_bCubeMap); // 是否是立方体贴图
	serializer.EndObject();

	serializer.SaveToFile(nxInfoPath.c_str());
}

void NXTexture::Deserialize()
{
	using namespace rapidjson;
	std::string nxInfoPath = m_texFilePath.string() + ".n0";
	NXDeserializer deserializer;
	bool bJsonExist = deserializer.LoadFromFile(nxInfoPath.c_str());
	if (bJsonExist)
	{
		//std::string strPathInfo;
		//strPathInfo = deserializer.String("NXInfoPath");
		m_serializationData.m_textureType = (NXTextureMode)deserializer.Int("TextureType");
		m_serializationData.m_bInvertNormalY = deserializer.Bool("IsInvertNormalY");
		m_serializationData.m_bGenerateMipMap = deserializer.Bool("IsGenerateMipMap");
		m_serializationData.m_bCubeMap = deserializer.Bool("IsCubeMap");
	}
}

Ntr<NXTexture2D> NXTexture2D::Create(const std::string& debugName, const std::filesystem::path& filePath, D3D12_RESOURCE_FLAGS flags)
{
	Deserialize();
	m_texFilePath = filePath;
	m_name = debugName;

	CreatePathTextureInternal(m_texFilePath, flags);
	return this;
}

Ntr<NXTexture2D> NXTexture2D::CreateRenderTexture(const std::string& debugName, DXGI_FORMAT fmt, uint32_t width, uint32_t height, D3D12_RESOURCE_FLAGS flags)
{
	m_texFilePath = "[Render Target: " + debugName + "]";
	m_name = debugName;
	m_width = width;
	m_height = height;
	m_arraySize = 1;
	m_mipLevels = 1;
	m_texFormat = fmt;

	CreateRenderTextureInternal(flags);

	return this;
}

Ntr<NXTexture2D> NXTexture2D::CreateSolid(const std::string& debugName, uint32_t texSize, const Vector4& color, D3D12_RESOURCE_FLAGS flags)
{
	// 创建大小为 texSize * texSize 的纯色纹理
	std::unique_ptr<ScratchImage> pImage = std::make_unique<ScratchImage>();
	DXGI_FORMAT fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
	pImage->Initialize2D(fmt, texSize, texSize, 1, 1);

	// Convert floating point color to 8-bit color
	uint8_t r = static_cast<uint8_t>(color.x * 255.0f);
	uint8_t g = static_cast<uint8_t>(color.y * 255.0f);
	uint8_t b = static_cast<uint8_t>(color.z * 255.0f);
	uint8_t a = static_cast<uint8_t>(color.w * 255.0f);

	const Image& pImg = *pImage->GetImage(0, 0, 0);
	for (uint32_t i = 0; i < texSize; ++i)
	{
		for (uint32_t j = 0; j < texSize; ++j)
		{
			uint8_t* pixel = pImg.pixels + i * pImg.rowPitch + j * 4;
			pixel[0] = r;
			pixel[1] = g;
			pixel[2] = b;
			pixel[3] = a;
		}
	}

	m_texFilePath = "[Default Solid Texture]";
	m_name = debugName;
	m_width = texSize;
	m_height = texSize;
	m_arraySize = 1;
	m_mipLevels = 1;
	m_texFormat = fmt;

	CreateInternal(std::move(pImage), flags);

	pImage.reset();
	return this;
}

Ntr<NXTexture2D> NXTexture2D::CreateNoise(const std::string& debugName, uint32_t texSize, uint32_t dimension, D3D12_RESOURCE_FLAGS flags)
{
	// 创建大小为 texSize * texSize 的噪声纹理

	// Check if dimension is valid (1, 2, 3, or 4)
	if (dimension < 1 || dimension > 4)
	{
		printf("Invalid Dimension for Noise Texture. Allowed values are 1, 2, 3, or 4.\n");
		return nullptr;
	}

	DXGI_FORMAT fmt;
	switch (dimension)
	{
	case 1: fmt = DXGI_FORMAT_R32_FLOAT; break;
	case 2: fmt = DXGI_FORMAT_R32G32_FLOAT; break;
	case 3: fmt = DXGI_FORMAT_R32G32B32_FLOAT; break;
	case 4: fmt = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
	}

	uint32_t bytePerPixel = sizeof(float) * dimension; // 2023.10.26 现阶段只支持 32-bit float 纹理，所以每个像素占 sizeof(float) * Dimension 字节

	std::unique_ptr<ScratchImage> pImage = std::make_unique<ScratchImage>();
	pImage->Initialize2D(fmt, texSize, texSize, 1, 1);

	NXRandom* randInst = NXRandom::GetInstance();
	const Image& image = *pImage->GetImage(0, 0, 0);
	for (uint32_t i = 0; i < texSize; ++i)
	{
		for (uint32_t j = 0; j < texSize; ++j)
		{
			float* pixel = reinterpret_cast<float*>(image.pixels + i * image.rowPitch + j * dimension * sizeof(float));
			for(uint32_t dim = 0; dim < dimension; dim++)
				pixel[dim] = randInst->CreateFloat();
		}
	}

	m_texFilePath = "[Default Noise Texture]";
	m_name = debugName;
	m_width = texSize;
	m_height = texSize;
	m_arraySize = 1;
	m_mipLevels = 1;
	m_texFormat = fmt;

	CreateInternal(std::move(pImage), flags);

	pImage.reset();
	return this;
}

void NXTexture2D::SetSRV(uint32_t index)
{
	NXAllocator_SRV->Alloc([this, index](const XShaderDescriptor& result) {
		m_pSRVs[index] = result;

		DXGI_FORMAT SRVFormat = m_texFormat;
		if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
			SRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // 默认的映射
		srvDesc.Format = SRVFormat; // 纹理的格式
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = m_mipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0;
		srvDesc.Texture2D.PlaneSlice = 0;

		NXGlobalDX::GetDevice()->CreateShaderResourceView(m_pTexture.Get(), &srvDesc, m_pSRVs[index].cpuHandle);

		ProcessLoadingBuffers();
		});
}

void NXTexture2D::SetRTV(uint32_t index)
{
	NXAllocator_RTV->Alloc([this, index](const XDescriptor& result) {
		m_pRTVs[index] = result;
		NXGlobalDX::GetDevice()->CreateRenderTargetView(m_pTexture.Get(), nullptr, m_pRTVs[index].cpuHandle);
		ProcessLoadingBuffers();
		});
}

void NXTexture2D::SetDSV(uint32_t index)
{
	NXAllocator_DSV->Alloc([this, index](const XDescriptor& result) {
		m_pDSVs[index] = result;

		DXGI_FORMAT DSVFormat = m_texFormat;
		if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
			DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		else if (m_texFormat == DXGI_FORMAT_R32_TYPELESS)
			DSVFormat = DXGI_FORMAT_D32_FLOAT;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DSVFormat;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;

		NXGlobalDX::GetDevice()->CreateDepthStencilView(m_pTexture.Get(), nullptr, m_pDSVs[index].cpuHandle);

		ProcessLoadingBuffers();
		});
}

void NXTexture2D::SetUAV(uint32_t index)
{
	NXAllocator_SRV->Alloc([this, index](const XShaderDescriptor& result) {
		m_pUAVs[index] = result;
		NXGlobalDX::GetDevice()->CreateUnorderedAccessView(m_pTexture.Get(), nullptr, nullptr, m_pUAVs[index].cpuHandle);
		ProcessLoadingBuffers();
		});
}

void NXTextureCube::Create(const std::string& debugName, DXGI_FORMAT texFormat, uint32_t width, uint32_t height, uint32_t mipLevels, D3D12_RESOURCE_FLAGS flags)
{
	m_name = debugName;
	m_width = width;
	m_height = height;
	m_arraySize = 6; 	// textureCube must be 6.
	m_texFormat = texFormat;
	m_mipLevels = mipLevels;

	CreateRenderTextureInternal(flags);
}

void NXTextureCube::Create(const std::string& debugName, const std::wstring& filePath, size_t width, size_t height, D3D12_RESOURCE_FLAGS flags)
{
	m_texFilePath = filePath.c_str();
	m_name = debugName;

	CreatePathTextureInternal(m_texFilePath, flags);

	SetSRVPreview2D();
}

void NXTextureCube::SetSRV(uint32_t index)
{
	NXAllocator_SRV->Alloc([this, index](const XShaderDescriptor& result) {
		m_pSRVs[index] = result;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = m_texFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.TextureCube.MipLevels = m_mipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

		NXGlobalDX::GetDevice()->CreateShaderResourceView(m_pTexture.Get(), &srvDesc, m_pSRVs[index].cpuHandle);

		ProcessLoadingBuffers();
		});
}

void NXTextureCube::SetSRVPreview2D()
{
	NXAllocator_SRV->Alloc([this](const XShaderDescriptor& result) {
		m_pSRVPreview2D = result;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = m_texFormat;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY; // Use a 2D texture array view
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.MipLevels = 1;
		srvDesc.Texture2DArray.FirstArraySlice = 0;
		srvDesc.Texture2DArray.ArraySize = 1;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;

		NXGlobalDX::GetDevice()->CreateShaderResourceView(m_pTexture.Get(), &srvDesc, m_pSRVPreview2D.cpuHandle);
		});
}

void NXTextureCube::SetRTV(uint32_t index, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize)
{
	NXAllocator_RTV->Alloc([this, index, mipSlice, firstArraySlice, arraySize](const XDescriptor& result) {
		m_pRTVs[index] = result;

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = m_texFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = mipSlice;
		rtvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		rtvDesc.Texture2DArray.ArraySize = arraySize;
		rtvDesc.Texture2DArray.PlaneSlice = 0;

		NXGlobalDX::GetDevice()->CreateRenderTargetView(m_pTexture.Get(), &rtvDesc, m_pRTVs[index].cpuHandle);

		ProcessLoadingBuffers();
		});
}

void NXTextureCube::SetDSV(uint32_t index, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize)
{
	NXAllocator_DSV->Alloc([this, index, mipSlice, firstArraySlice, arraySize](const XDescriptor& result) {
		m_pDSVs[index] = result;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Format = m_texFormat;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Texture2DArray.MipSlice = mipSlice;
		dsvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		dsvDesc.Texture2DArray.ArraySize = arraySize;

		NXGlobalDX::GetDevice()->CreateDepthStencilView(m_pTexture.Get(), &dsvDesc, m_pDSVs[index].cpuHandle);

		ProcessLoadingBuffers();
		});
}

void NXTextureCube::SetUAV(uint32_t index, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize)
{
	NXAllocator_SRV->Alloc([this, index, mipSlice, firstArraySlice, arraySize](const XShaderDescriptor& result) {
		m_pUAVs[index] = result;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = m_texFormat;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.MipSlice = mipSlice;
		uavDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		uavDesc.Texture2DArray.ArraySize = arraySize;
		uavDesc.Texture2DArray.PlaneSlice = 0;

		NXGlobalDX::GetDevice()->CreateUnorderedAccessView(m_pTexture.Get(), nullptr, &uavDesc, m_pUAVs[index].cpuHandle);

		ProcessLoadingBuffers();
		});
}

void NXTexture2DArray::Create(const std::string& debugName, DXGI_FORMAT texFormat, uint32_t width, uint32_t height, uint32_t arraySize, uint32_t mipLevels, D3D12_RESOURCE_FLAGS flags)
{
	this->m_name = debugName;
	this->m_width = width;
	this->m_height = height;
	this->m_arraySize = arraySize;
	this->m_texFormat = texFormat;
	this->m_mipLevels = mipLevels;
	CreateRenderTextureInternal(flags);
}

void NXTexture2DArray::SetSRV(uint32_t index, uint32_t firstArraySlice, uint32_t arraySize)
{
	NXAllocator_SRV->Alloc([this, index, firstArraySlice, arraySize](const XShaderDescriptor& result) {
		m_pSRVs[index] = result;

		DXGI_FORMAT SRVFormat = m_texFormat;
		if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
			SRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		else if (m_texFormat == DXGI_FORMAT_R32_TYPELESS)
			SRVFormat = DXGI_FORMAT_R32_FLOAT;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // 默认的映射
		srvDesc.Format = SRVFormat; // 纹理的格式
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		srvDesc.Texture2DArray.MipLevels = m_mipLevels;
		srvDesc.Texture2DArray.MostDetailedMip = 0;
		srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0;
		srvDesc.Texture2DArray.PlaneSlice = 0;
		srvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		srvDesc.Texture2DArray.ArraySize = arraySize;

		NXGlobalDX::GetDevice()->CreateShaderResourceView(m_pTexture.Get(), &srvDesc, m_pSRVs[index].cpuHandle);

		ProcessLoadingBuffers();
		});
}

void NXTexture2DArray::SetRTV(uint32_t index, uint32_t firstArraySlice, uint32_t arraySize)
{
	NXAllocator_RTV->Alloc([this, index, firstArraySlice, arraySize](const XDescriptor& result) {
		m_pRTVs[index] = result;

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = m_texFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		rtvDesc.Texture2DArray.ArraySize = arraySize;
		rtvDesc.Texture2DArray.PlaneSlice = 0;

		NXGlobalDX::GetDevice()->CreateRenderTargetView(m_pTexture.Get(), &rtvDesc, m_pRTVs[index].cpuHandle);

		ProcessLoadingBuffers();
		});
}

void NXTexture2DArray::SetDSV(uint32_t index, uint32_t firstArraySlice, uint32_t arraySize)
{
	NXAllocator_DSV->Alloc([this, index, firstArraySlice, arraySize](const XDescriptor& result) {
		m_pDSVs[index] = result;

		DXGI_FORMAT DSVFormat = m_texFormat;
		if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
			DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		else if (m_texFormat == DXGI_FORMAT_R32_TYPELESS)
			DSVFormat = DXGI_FORMAT_D32_FLOAT;

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DSVFormat;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		dsvDesc.Texture2DArray.ArraySize = arraySize;

		NXGlobalDX::GetDevice()->CreateDepthStencilView(m_pTexture.Get(), &dsvDesc, m_pDSVs[index].cpuHandle);

		ProcessLoadingBuffers();
		});
}

void NXTexture2DArray::SetUAV(uint32_t index, uint32_t firstArraySlice, uint32_t arraySize)
{
	NXAllocator_SRV->Alloc([this, index, firstArraySlice, arraySize](const XShaderDescriptor& result) {
		m_pUAVs[index] = result;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = m_texFormat;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Texture2DArray.MipSlice = 0;
		uavDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
		uavDesc.Texture2DArray.ArraySize = arraySize;

		NXGlobalDX::GetDevice()->CreateUnorderedAccessView(m_pTexture.Get(), nullptr, &uavDesc, m_pUAVs[index].cpuHandle);

		ProcessLoadingBuffers();
		});
}