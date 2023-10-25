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

void NXTexture2D::Create(std::string DebugName, const D3D11_SUBRESOURCE_DATA* initData, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT ArraySize, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
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

	this->m_texFilePath = filePath;
	this->m_name = DebugName;
	this->m_width = (UINT)metadata.width;
	this->m_height = (UINT)metadata.height;
	this->m_arraySize = (UINT)metadata.arraySize;
	this->m_mipLevels = (UINT)metadata.mipLevels;
	this->m_texFormat = metadata.format;

	DirectX::CreateTextureEx(g_pDevice.Get(), pImage->GetImage(0, 0, 0), pImage->GetImageCount(), metadata, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, false, CREATETEX_DEFAULT, (ID3D11Resource**)m_pTexture.GetAddressOf());
	m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DebugName.size(), DebugName.c_str());

	pImage.reset();
	return this;
}

Ntr<NXTexture2D> NXTexture2D::CreateSolid(const std::string& DebugName, UINT TexSize, const Vector4& Color)
{
	// 创建大小为 TexSize * TexSize 的纯色纹理
	DirectX::Image image;
	image.width = TexSize;
	image.height = TexSize;
	image.format = DXGI_FORMAT_R8G8B8A8_UNORM;		// 8-bit UNORM for each channel
	image.rowPitch = TexSize * 4;					// 4 bytes per pixel (RGBA)
	image.slicePitch = image.rowPitch * TexSize;	// size of entire image
	image.pixels = new uint8_t[image.slicePitch];

	// Convert floating point color to 8-bit color
	uint8_t r = static_cast<uint8_t>(Color.x * 255.0f);
	uint8_t g = static_cast<uint8_t>(Color.y * 255.0f);
	uint8_t b = static_cast<uint8_t>(Color.z * 255.0f);
	uint8_t a = static_cast<uint8_t>(Color.w * 255.0f);

	for (UINT i = 0; i < TexSize; ++i)
	{
		for (UINT j = 0; j < TexSize; ++j)
		{
			uint8_t* pixel = image.pixels + i * image.rowPitch + j * 4;
			pixel[0] = r;
			pixel[1] = g;
			pixel[2] = b;
			pixel[3] = a;
		}
	}

	// Directly create the texture using the provided data
	TexMetadata metadata;
	metadata.width = image.width;
	metadata.height = image.height;
	metadata.depth = 1;
	metadata.arraySize = 1;
	metadata.mipLevels = 1;
	metadata.format = image.format;
	metadata.dimension = TEX_DIMENSION_TEXTURE2D;

	DirectX::CreateTextureEx(g_pDevice.Get(), &image, 1, metadata, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, CREATETEX_DEFAULT, (ID3D11Resource**)m_pTexture.GetAddressOf());
	m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DebugName.size(), DebugName.c_str());

	delete[] image.pixels;
	return this;
}

Ntr<NXTexture2D> NXTexture2D::CreateNoise(const std::string& DebugName, UINT TexSize, UINT Dimension)
{
	// Check if dimension is valid (1, 2, 3, or 4)
	if (Dimension < 1 || Dimension > 4)
	{
		printf("Invalid Dimension for Noise Texture. Allowed values are 1, 2, 3, or 4.\n");
		return nullptr;
	}

	DXGI_FORMAT format;
	switch (Dimension)
	{
	case 1: format = DXGI_FORMAT_R32_FLOAT; break;
	case 2: format = DXGI_FORMAT_R32G32_FLOAT; break;
	case 3: format = DXGI_FORMAT_R32G32B32_FLOAT; break;
	case 4: format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
	}

	UINT bytePerPixel = 4 * Dimension; // 现阶段只支持 32-bit 纹理，每个像素 4 * Dimension 字节

	DirectX::Image image;
	image.width = TexSize;
	image.height = TexSize;
	image.format = format;
	image.rowPitch = TexSize * bytePerPixel;
	image.slicePitch = image.rowPitch * TexSize;
	image.pixels = new uint8_t[image.slicePitch];

	NXRandom* randInst = NXRandom::GetInstance();
	for (UINT i = 0; i < TexSize; ++i)
	{
		for (UINT j = 0; j < TexSize; ++j)
		{
			uint8_t* pixel = image.pixels + i * image.rowPitch + j * Dimension;
			for(UINT dim = 0; dim < Dimension; dim++)
				pixel[dim] = randInst->CreateUINT8();
		}
	}

	// Directly create the texture using the provided data
	TexMetadata metadata;
	metadata.width = image.width;
	metadata.height = image.height;
	metadata.depth = 1;
	metadata.arraySize = 1;
	metadata.mipLevels = 1;
	metadata.format = image.format;
	metadata.dimension = TEX_DIMENSION_TEXTURE2D;
	metadata.miscFlags = 0;
	metadata.miscFlags2 = 0;

	DirectX::CreateTextureEx(g_pDevice.Get(), &image, 1, metadata, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, CREATETEX_DEFAULT, (ID3D11Resource**)m_pTexture.GetAddressOf());
	m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DebugName.size(), DebugName.c_str());

	delete[] image.pixels;
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

void NXTextureCube::Create(std::string DebugName, const D3D11_SUBRESOURCE_DATA* initData, DXGI_FORMAT TexFormat, UINT Width, UINT Height, UINT MipLevels, UINT BindFlags, D3D11_USAGE Usage, UINT CpuAccessFlags, UINT SampleCount, UINT SampleQuality, UINT MiscFlags)
{
	UINT ArraySize = 6;	// textureCube must be 6.

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
	Desc.MiscFlags = MiscFlags | D3D11_RESOURCE_MISC_TEXTURECUBE; // textureCube must keep D3D11_RESOURCE_MISC_TEXTURECUBE flag.

	NX::ThrowIfFailed(g_pDevice->CreateTexture2D(&Desc, initData, &m_pTexture));
	m_pTexture->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)DebugName.size(), DebugName.c_str());
}

void NXTextureCube::Create(const std::string& DebugName, const std::wstring& FilePath, size_t width, size_t height)
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
	this->m_name = DebugName;
	this->m_width = (UINT)metadata.width;
	this->m_height = (UINT)metadata.height;
	this->m_arraySize = (UINT)metadata.arraySize;
	this->m_texFormat = metadata.format;
	this->m_mipLevels = (UINT)metadata.mipLevels;

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
