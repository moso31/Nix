#include "NXTextureLoader.h"
#include "NXConverter.h"

NXTextureLoader::NXTextureLoader()
{
}

NXTextureLoader::~NXTextureLoader()
{
}

void NXTextureLoader::AddTask(const NXTextureLoaderTask& task)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_tasks.push_back(task);
}

void NXTextureLoader::Update()
{
	std::vector<NXTextureLoaderTask> tasks;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		tasks.swap(m_tasks);
	}

	for (auto& task : tasks)
	{
		DoTask(task);
	}
	tasks.clear();
}

void NXTextureLoader::DoTask(const NXTextureLoaderTask& task)
{
	auto& filePath = task.path;
	auto& type = task.type;
	auto& serializationData = task.serializationData;

	NXTextureLoaderTaskResult result;
	TexMetadata& metadata = result.metadata;
	std::shared_ptr<ScratchImage>& pImage = result.pImage;
	pImage = std::make_shared<ScratchImage>();

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
	if (metadata.arraySize > 1 && type == NXResourceType::Tex2D)
	{
		std::shared_ptr<ScratchImage> timage(new ScratchImage);
		hr = timage->InitializeFromImage(*pImage->GetImage(0, 0, 0));
		if (SUCCEEDED(hr))
		{
			metadata = timage->GetMetadata();
		}
		else
		{
			printf("Warning: [InitializeFromImage] failed when loading NXTexture2D: %s\n", filePath.string().c_str());
		}
		pImage.swap(timage);
	}

	if (NXConvert::IsUnormFormat(metadata.format))
	{
		DXGI_FORMAT safeFormat = NXConvert::SafeDXGIFormat(metadata.format);
		if (metadata.format != safeFormat)
		{
			std::shared_ptr<ScratchImage> timage(new ScratchImage);
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
	if (serializationData.m_textureType == NXTextureMode::sRGB || serializationData.m_textureType == NXTextureMode::Linear)
	{
		bool bIsSRGB = serializationData.m_textureType == NXTextureMode::sRGB;
		DXGI_FORMAT tFormat = bIsSRGB ? NXConvert::ForceSRGB(metadata.format) : NXConvert::ForceLinear(metadata.format);
		if (metadata.format != tFormat)
		{
			std::shared_ptr<ScratchImage> timage(new ScratchImage);

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
	if (serializationData.m_bInvertNormalY)
	{
		std::shared_ptr<ScratchImage> timage(new ScratchImage);

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

	if (serializationData.m_bGenerateMipMap && metadata.width >= 2 && metadata.height >= 2 && metadata.mipLevels == 1)
	{
		std::shared_ptr<ScratchImage> pImageMip = std::make_shared<ScratchImage>();
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

	task.pCallBack(std::move(result));
}
