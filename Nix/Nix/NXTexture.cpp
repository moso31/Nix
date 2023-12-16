#include "NXTexture.h"
#include "Global.h"
#include "DirectXTex.h"
#include "NXResourceManager.h"
#include "NXConverter.h"
#include "NXLog.h"
#include "NXRandom.h"

NXTexture::~NXTexture()
{
	//NXLog::LogWithStackTrace("[%s : size=(%dx%d)x%d, mip=%d, path=%s] Deleted. remain RefCount: %d", m_name.c_str(), m_width, m_height, m_arraySize, m_mipLevels, m_texFilePath.string().c_str(), m_nRefCount);
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

void NXTexture::CreateInternal(const std::string& debugName, const std::unique_ptr<DirectX::ScratchImage>& pImage)
{
	// TODO：关于纹理创建这事比较麻烦，DX12现在需要一个uploadBuffer做中继，不然传不上去
	// 但是 uploadBuffer 在数据上传后，就没用了，关键是如何确认uploadBuffer 在GPU中上传完毕，可以释放？
	// 常用的做法包括：重用上传缓冲区，或基于Fence释放资源，这都是可以考虑的方向。
	// 为了实现这个，可能需要配套设计一个异步管理系统。太繁琐了，我还没想好。
	// 2023.12.11
	// 目前处于11转12阶段，为方便起见，暂时在NXTexture中始终保持这两份内存。
	// 好处是简单，坏处是额外存储了一份纹理的内存开销。必然要改
	std::wstring name = NXConvert::s2ws(debugName);
	std::wstring nameUploadTemp = name + L"UploadBuffer temp";
	m_pTexture = NX12Util::CreateTexture2D(g_pDevice.Get(), name.c_str(), m_width, m_height, m_texFormat, m_mipLevels, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_COPY_DEST);

	UINT layoutSize = m_arraySize * m_mipLevels;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = new D3D12_PLACED_SUBRESOURCE_FOOTPRINT[layoutSize];
	UINT* pNumRows = new UINT[layoutSize];
	UINT64* pRowSizeInBytes = new UINT64[layoutSize];

	UINT uploadBufferSize = NX12Util::GetRequiredIntermediateLayoutInfos(g_pDevice.Get(), m_pTexture.Get(), pLayouts, pNumRows, pRowSizeInBytes);
	m_pTextureUploadBuffer = NX12Util::CreateBuffer(g_pDevice.Get(), nameUploadTemp.c_str(), uploadBufferSize, D3D12_HEAP_TYPE_UPLOAD);

	void* pMappedData;
	m_pTextureUploadBuffer->Map(0, nullptr, &pMappedData);

	for (UINT item = 0, index = 0; item < m_arraySize; item++)
	{
		for (UINT mip = 0; mip < m_mipLevels; mip++, index++)
		{
			const Image* pImg = pImage->GetImage(mip, item, 0);
			const BYTE* pSrcData = pImg->pixels;
			BYTE* pDstData = reinterpret_cast<BYTE*>(pMappedData) + pLayouts[index].Offset;

			for (UINT y = 0; y < pNumRows[index]; y++)
			{
				memcpy(pDstData + pLayouts[index].Footprint.RowPitch * y, pSrcData + pImg->rowPitch * y, pRowSizeInBytes[index]);
			}
		}
	}
	m_pTextureUploadBuffer->Unmap(0, nullptr);

	NX12Util::CopyTextureRegion(g_pCommandList.Get(), m_pTexture.Get(), m_pTextureUploadBuffer.Get(), layoutSize, pLayouts);

	auto barrier = NX12Util::CreateResourceBarrier_Transition(m_pTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	g_pCommandList->ResourceBarrier(1, &barrier);

	delete[] pLayouts;
	delete[] pNumRows;
	delete[] pRowSizeInBytes;
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
		m_serializationData.m_textureType = (NXTextureType)deserializer.Int("TextureType");
		m_serializationData.m_bInvertNormalY = deserializer.Bool("IsInvertNormalY");
		m_serializationData.m_bGenerateMipMap = deserializer.Bool("IsGenerateMipMap");
		m_serializationData.m_bCubeMap = deserializer.Bool("IsCubeMap");
	}
}

Ntr<NXTexture2D> NXTexture2D::Create(const std::string& DebugName, const std::filesystem::path& filePath)
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
		pImage.reset();
		return nullptr;
	}

	m_texFilePath = filePath;

	Deserialize();

	// 如果读取的是arraySize/TextureCube，就只读取ArraySize[0]/X+面。
	if (metadata.arraySize > 1)
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
				printf("Warning: [Convert] failed when loading NXTexture2D: %s.\n", filePath.string().c_str());
			}
			pImage.swap(timage);
		}
	}

	// 如果序列化的文件里记录了sRGB/Linear类型，就做对应的转换
	if (m_serializationData.m_textureType == NXTextureType::sRGB || m_serializationData.m_textureType == NXTextureType::Linear)
	{
		bool bIsSRGB = m_serializationData.m_textureType == NXTextureType::sRGB;
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
				printf("Warning: [Convert] failed when loading NXTexture2D: %s\n", filePath.string().c_str());
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
			printf("Warning: [InvertNormalY] failed when loading NXTexture2D: %s\n", filePath.string().c_str());
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
			printf("Warning: [GenerateMipMap] failed when loading NXTexture2D: %s\n", filePath.string().c_str());
		}
	}

	m_texFilePath = filePath;
	m_name = DebugName;
	m_width = (UINT)metadata.width;
	m_height = (UINT)metadata.height;
	m_arraySize = (UINT)metadata.arraySize;
	m_mipLevels = (UINT)metadata.mipLevels;
	m_texFormat = metadata.format;

	CreateInternal(DebugName, pImage);

	pImage.reset();
	return this;
}

Ntr<NXTexture2D> NXTexture2D::CreateRT(const std::string& debugName, DXGI_FORMAT fmt, UINT width, UINT height)
{
	m_pTexture = NX12Util::CreateTexture2D(g_pDevice.Get(), debugName, width, height, fmt, 1, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	m_texFilePath = "[Render Target: " + debugName + "]";
	m_name = debugName;
	m_width = width;
	m_height = height;
	m_arraySize = 1;
	m_mipLevels = 1;
	m_texFormat = fmt;

	return this;
}

Ntr<NXTexture2D> NXTexture2D::CreateSolid(const std::string& debugName, UINT texSize, const Vector4& color)
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
	for (UINT i = 0; i < texSize; ++i)
	{
		for (UINT j = 0; j < texSize; ++j)
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

	CreateInternal(debugName, pImage);

	pImage.reset();
	return this;
}

Ntr<NXTexture2D> NXTexture2D::CreateNoise(const std::string& debugName, UINT texSize, UINT dimension)
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

	UINT bytePerPixel = sizeof(float) * dimension; // 2023.10.26 现阶段只支持 32-bit float 纹理，所以每个像素占 sizeof(float) * Dimension 字节

	std::unique_ptr<ScratchImage> pImage = std::make_unique<ScratchImage>();
	pImage->Initialize2D(fmt, texSize, texSize, 1, 1);

	NXRandom* randInst = NXRandom::GetInstance();
	const Image& image = *pImage->GetImage(0, 0, 0);
	for (UINT i = 0; i < texSize; ++i)
	{
		for (UINT j = 0; j < texSize; ++j)
		{
			float* pixel = reinterpret_cast<float*>(image.pixels + i * image.rowPitch + j * dimension * sizeof(float));
			for(UINT dim = 0; dim < dimension; dim++)
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

	CreateInternal(debugName, pImage);

	pImage.reset();
	return this;
}

void NXTexture2D::AddSRV()
{
	ComPtr<ID3D11ShaderResourceView> pSRV;

	DXGI_FORMAT SRVFormat = m_texFormat;
	if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
		SRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

	CD3D11_SHADER_RESOURCE_VIEW_DESC Desc(D3D11_SRV_DIMENSION_TEXTURE2D, SRVFormat, 0, m_mipLevels);
	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(m_pTexture.Get(), &Desc, &pSRV));

	std::string SRVDebugName = m_name + " SRV";
	pSRV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)SRVDebugName.size(), SRVDebugName.c_str());

	m_pSRVs.push_back(pSRV);
}

void NXTexture2D::AddRTV()
{
	ComPtr<ID3D11RenderTargetView> pRTV;

	CD3D11_RENDER_TARGET_VIEW_DESC Desc(D3D11_RTV_DIMENSION_TEXTURE2D, m_texFormat);
	NX::ThrowIfFailed(g_pDevice->CreateRenderTargetView(m_pTexture.Get(), &Desc, &pRTV));

	std::string RTVDebugName = m_name + " RTV";
	pRTV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)RTVDebugName.size(), RTVDebugName.c_str());

	m_pRTVs.push_back(pRTV);
}

void NXTexture2D::AddDSV()
{
	ComPtr<ID3D11DepthStencilView> pDSV;

	DXGI_FORMAT DSVFormat = m_texFormat;
	if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
		DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	CD3D11_DEPTH_STENCIL_VIEW_DESC Desc(D3D11_DSV_DIMENSION_TEXTURE2D, DSVFormat);
	NX::ThrowIfFailed(g_pDevice->CreateDepthStencilView(m_pTexture.Get(), &Desc, &pDSV));

	std::string DSVDebugName = m_name + " DSV";
	pDSV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DSVDebugName.size(), DSVDebugName.c_str());

	m_pDSVs.push_back(pDSV);
}

void NXTexture2D::AddUAV()
{
	ComPtr<ID3D11UnorderedAccessView> pUAV;

	DXGI_FORMAT UAVFormat = m_texFormat;

	CD3D11_UNORDERED_ACCESS_VIEW_DESC Desc(D3D11_UAV_DIMENSION_TEXTURE2D, UAVFormat);
	NX::ThrowIfFailed(g_pDevice->CreateUnorderedAccessView(m_pTexture.Get(), &Desc, &pUAV));

	std::string UAVDebugName = m_name + " UAV";
	pUAV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)UAVDebugName.size(), UAVDebugName.c_str());

	m_pUAVs.push_back(pUAV);
}

void NXTextureCube::Create(std::string debugName, const D3D11_SUBRESOURCE_DATA* initData, DXGI_FORMAT texFormat, UINT width, UINT height, UINT miplevels, UINT bindflags, D3D11_USAGE usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	UINT ArraySize = 6;	// textureCube must be 6.

	m_name = debugName;
	m_width = Width;
	m_height = Height;
	m_arraySize = ArraySize;
	m_texFormat = TexFormat;
	m_mipLevels = MipLevels;

	NX12Util::CreateTexture2D(g_pDevice.Get(), debugName, width, height, texf)

	D3D11_TEXTURE2D_DESC Desc;
	Desc.Format = TexFormat;
	Desc.Width = Width;
	Desc.Height = Height;
	Desc.ArraySize = ArraySize;
	Desc.MipLevels = MipLevels;
	Desc.BindFlags = BindFlags;
	Desc.Usage = Usage;
	Desc.CPUAccessFlags = CpuAccessFlags;
	Desc.SampleDesc.Count = SampleCount;
	Desc.SampleDesc.Quality = SampleQuality;
	Desc.MiscFlags = MiscFlags | D3D11_RESOURCE_MISC_TEXTURECUBE; // textureCube must keep D3D11_RESOURCE_MISC_TEXTURECUBE flag.

	NX::ThrowIfFailed(g_pDevice->CreateTexture2D(&Desc, initData, &m_pTexture));
	m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DebugName.size(), DebugName.c_str());
}

void NXTextureCube::Create(const std::string& debugName, const std::wstring& FilePath, size_t width, size_t height)
{
	TexMetadata metadata;
	std::unique_ptr<ScratchImage> pImage = std::make_unique<ScratchImage>();
	LoadFromDDSFile(FilePath.c_str(), DDS_FLAGS_NONE, &metadata, *pImage);
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
			printf("Warning: [Decompress] failure when loading NXTextureCube file: %ws\n", FilePath.c_str());
		}
	}

	bool bResize = width && height && width != metadata.width && height != metadata.height;
	if (bResize)
	{
		std::unique_ptr<ScratchImage> timage = std::make_unique<ScratchImage>();
		HRESULT hr = Resize(pImage->GetImages(), pImage->GetImageCount(), pImage->GetMetadata(), width, height, TEX_FILTER_DEFAULT, *timage);
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
			printf("Warning: [Resize] failure when loading NXTextureCube file: %ws\n", FilePath.c_str());
		}
	}

	bool bGenerateMipMap = false;
	if (bGenerateMipMap)
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
			printf("Warning: [GenerateMipMaps] failure when loading NXTextureCube file: %ws\n", FilePath.c_str());
		}
	}

	this->m_texFilePath = FilePath.c_str();
	this->m_name = debugName;
	this->m_width = (UINT)metadata.width;
	this->m_height = (UINT)metadata.height;
	this->m_arraySize = (UINT)metadata.arraySize;
	this->m_texFormat = metadata.format;
	this->m_mipLevels = (UINT)metadata.mipLevels;

	CreateInternal(debugName, pImage);

	DirectX::CreateTextureEx(g_pDevice.Get(), pImage->GetImages(), pImage->GetImageCount(), metadata, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE, CREATETEX_DEFAULT, (ID3D11Resource**)m_pTexture.GetAddressOf());
	m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DebugName.size(), DebugName.c_str());

	auto HDRPreviewInfo = metadata;
	HDRPreviewInfo.arraySize = 1;
	HDRPreviewInfo.mipLevels = 1;
	HDRPreviewInfo.miscFlags = 0;
	CreateShaderResourceView(g_pDevice.Get(), pImage->GetImage(0, 0, 0), 1, HDRPreviewInfo, &m_pSRVPreview2D);
}

void NXTextureCube::AddSRV()
{
	ComPtr<ID3D11ShaderResourceView> pSRV;

	D3D11_SHADER_RESOURCE_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	Desc.Format = m_texFormat;
	Desc.TextureCube.MostDetailedMip = 0;
	Desc.TextureCube.MipLevels = m_mipLevels;

	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(m_pTexture.Get(), &Desc, &pSRV));

	std::string SRVDebugName = m_name + " SRV";
	pSRV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)SRVDebugName.size(), SRVDebugName.c_str());

	m_pSRVs.push_back(pSRV);
}

void NXTextureCube::AddRTV(UINT mipSlice, UINT firstArraySlice, UINT arraySize)
{
	ComPtr<ID3D11RenderTargetView> pRTV;

	D3D11_RENDER_TARGET_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format = m_texFormat;
	Desc.Texture2DArray.MipSlice = mipSlice;
	Desc.Texture2DArray.FirstArraySlice = firstArraySlice;
	Desc.Texture2DArray.ArraySize = arraySize;

	NX::ThrowIfFailed(g_pDevice->CreateRenderTargetView(m_pTexture.Get(), &Desc, &pRTV));

	std::string RTVDebugName = m_name + " RTV";
	pRTV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)RTVDebugName.size(), RTVDebugName.c_str());

	m_pRTVs.push_back(pRTV);
}

void NXTextureCube::AddDSV(UINT mipSlice, UINT firstArraySlice, UINT arraySize)
{
	D3D11_DEPTH_STENCIL_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format = m_texFormat;
	Desc.Texture2DArray.MipSlice = mipSlice;
	Desc.Texture2DArray.FirstArraySlice = firstArraySlice;
	Desc.Texture2DArray.ArraySize = arraySize;
}

void NXTextureCube::AddUAV(UINT mipSlice, UINT firstArraySlice, UINT arraySize)
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format = m_texFormat;
	Desc.Texture2DArray.MipSlice = mipSlice;
	Desc.Texture2DArray.FirstArraySlice = firstArraySlice;
	Desc.Texture2DArray.ArraySize = arraySize;
}

void NXTexture2DArray::Create(std::string DebugName, const D3D11_SUBRESOURCE_DATA* initData, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	this->m_name = DebugName;
	this->m_width = Width;
	this->m_height = Height;
	this->m_arraySize = ArraySize;
	this->m_texFormat = TexFormat;
	this->m_mipLevels = MipLevels;

	D3D11_TEXTURE2D_DESC Desc;
	Desc.Format = TexFormat;
	Desc.Width = Width;
	Desc.Height = Height;
	Desc.ArraySize = ArraySize;
	Desc.MipLevels = MipLevels;
	Desc.BindFlags = BindFlags;
	Desc.Usage = Usage;
	Desc.CPUAccessFlags = CpuAccessFlags;
	Desc.SampleDesc.Count = SampleCount;
	Desc.SampleDesc.Quality = SampleQuality;
	Desc.MiscFlags = MiscFlags;

	NX::ThrowIfFailed(g_pDevice->CreateTexture2D(&Desc, initData, &m_pTexture));
	m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DebugName.size(), DebugName.c_str());
}

void NXTexture2DArray::AddSRV(UINT firstArraySlice, UINT arraySize)
{
	ComPtr<ID3D11ShaderResourceView> pSRV;

	DXGI_FORMAT SRVFormat = m_texFormat;
	if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
		SRVFormat = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	else if (m_texFormat == DXGI_FORMAT_R32_TYPELESS)
		SRVFormat = DXGI_FORMAT_R32_FLOAT;

	D3D11_SHADER_RESOURCE_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format = SRVFormat;
	Desc.Texture2DArray.MostDetailedMip = 0;
	Desc.Texture2DArray.MipLevels = m_mipLevels;
	Desc.Texture2DArray.FirstArraySlice = firstArraySlice;
	Desc.Texture2DArray.ArraySize = arraySize;

	NX::ThrowIfFailed(g_pDevice->CreateShaderResourceView(m_pTexture.Get(), &Desc, &pSRV));

	std::string SRVDebugName = m_name + " SRV";
	pSRV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)SRVDebugName.size(), SRVDebugName.c_str());

	m_pSRVs.push_back(pSRV);
}

void NXTexture2DArray::AddRTV(UINT firstArraySlice, UINT arraySize)
{
	ComPtr<ID3D11RenderTargetView> pRTV;

	D3D11_RENDER_TARGET_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format = m_texFormat;
	Desc.Texture2DArray.MipSlice = 0;
	Desc.Texture2DArray.FirstArraySlice = firstArraySlice;
	Desc.Texture2DArray.ArraySize = arraySize;

	NX::ThrowIfFailed(g_pDevice->CreateRenderTargetView(m_pTexture.Get(), &Desc, &pRTV));

	std::string RTVDebugName = m_name + " RTV";
	pRTV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)RTVDebugName.size(), RTVDebugName.c_str());

	m_pRTVs.push_back(pRTV);
}

void NXTexture2DArray::AddDSV(UINT firstArraySlice, UINT arraySize)
{
	ComPtr<ID3D11DepthStencilView> pDSV;

	DXGI_FORMAT DSVFormat = m_texFormat;
	if (m_texFormat == DXGI_FORMAT_R24G8_TYPELESS)
		DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	else if (m_texFormat == DXGI_FORMAT_R32_TYPELESS)
		DSVFormat = DXGI_FORMAT_D32_FLOAT;

	D3D11_DEPTH_STENCIL_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format = DSVFormat;
	Desc.Texture2DArray.FirstArraySlice = firstArraySlice;
	Desc.Texture2DArray.ArraySize = arraySize;
	Desc.Texture2DArray.MipSlice = 0;
	Desc.Flags = 0;

	NX::ThrowIfFailed(g_pDevice->CreateDepthStencilView(m_pTexture.Get(), &Desc, &pDSV));

	std::string DSVDebugName = m_name + " DSV";
	pDSV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DSVDebugName.size(), DSVDebugName.c_str());

	m_pDSVs.push_back(pDSV);
}

void NXTexture2DArray::AddUAV(UINT firstArraySlice, UINT arraySize)
{
	ComPtr<ID3D11UnorderedAccessView> pUAV;

	DXGI_FORMAT UAVFormat = m_texFormat;

	D3D11_UNORDERED_ACCESS_VIEW_DESC Desc;
	Desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
	Desc.Format = UAVFormat;
	Desc.Texture2DArray.FirstArraySlice = firstArraySlice;
	Desc.Texture2DArray.ArraySize = arraySize;
	Desc.Texture2DArray.MipSlice = 0;

	NX::ThrowIfFailed(g_pDevice->CreateUnorderedAccessView(m_pTexture.Get(), &Desc, &pUAV));

	std::string UAVDebugName = m_name + " UAV";
	pUAV->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)UAVDebugName.size(), UAVDebugName.c_str());

	m_pUAVs.push_back(pUAV);
}
