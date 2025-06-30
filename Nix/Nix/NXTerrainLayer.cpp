#include "NXConverter.h"
#include "NXTerrainLayer.h"
#include "NXResourceManager.h"
#include "NXTexture.h"

NXTerrainLayer::NXTerrainLayer(const std::string& name) :
	m_name(name)
{
}

void NXTerrainLayer::SetPath(const std::filesystem::path& path, bool bForceCreate)
{
	m_path = path;

	if (!std::filesystem::exists(path) && bForceCreate)
	{
		// create directory 
		std::filesystem::path dir = path.parent_path();
		std::filesystem::create_directories(dir);

		// create ntl file
		std::ofstream ofs(path);
		return; 
	}

	Deserialize();
}

void NXTerrainLayer::SetHeightMapPath(const std::filesystem::path& heightMapPath)
{
	m_heightMapPath = heightMapPath;
	m_heightMapTexture = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("", heightMapPath);
	GenerateMinMaxZMap();
}

void NXTerrainLayer::Serialize()
{
	if (m_path.empty())
	{
		printf("Warning, %s couldn't be serialized, cause path %s does not exist.\n", m_path.string().c_str(), m_path.string().c_str());
		return;
	}

	NXSerializer serializer;
	std::string nxInfoPath = m_path.string() + ".n0";
	if (NXConvert::IsTerrainLayerExtension(m_path.extension().string()))
	{
		serializer.StartObject();
		serializer.String("path", m_path.string());
		serializer.String("heightMapPath", m_heightMapPath.string());
		serializer.String("minMaxZMapPath", m_minMaxZMapPath.string());
		serializer.EndObject();

		serializer.SaveToFile(m_path);
	}
}

void NXTerrainLayer::Deserialize()
{
	std::string nxInfoPath = m_path.string();

	NXDeserializer deserializer;
	bool bJsonExist = deserializer.LoadFromFile(nxInfoPath.c_str());
	if (bJsonExist)
	{
		m_path = deserializer.String("path", "");
		m_heightMapPath = deserializer.String("heightMapPath", "");
		m_minMaxZMapPath = deserializer.String("minMaxZMapPath", "");

		m_heightMapTexture = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("", m_heightMapPath);

		BakeGPUDrivenData(false);
	}
	else
	{ 
		// do nothing
	}
}

void NXTerrainLayer::Release()
{
}

void NXTerrainLayer::BakeGPUDrivenData(bool bSave)
{
	if (m_heightMapTexture.IsValid())
	{
		// 如果有高度图，但没有路径，说明还没生成过GPUDriven关联图，生成下
		if (!std::filesystem::exists(m_minMaxZMapPath))
		{
			GenerateMinMaxZMap();
		}

		m_minMaxZMapTexture = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("Terrain Patcher MinMaxZ", m_minMaxZMapPath);
	}

	if (bSave)
	{
		Serialize();
	}
}

void NXTerrainLayer::GenerateMinMaxZMap()
{
	auto rawPath = m_heightMapPath;
	uint32_t width = m_heightMapTexture->GetWidth();
	uint32_t height = m_heightMapTexture->GetHeight();
	std::vector<uint16_t> rawData(width * height);

	// 读取rawPath的文件，并转换成单通道纹理
	std::ifstream file(rawPath, std::ios::binary);
	if (!file)
		throw std::runtime_error("无法打开文件: " + rawPath.string());

	// 直接读数据就行，必须是16bit 
	// todo: 支持更多格式
	file.read(reinterpret_cast<char*>(rawData.data()), width * height * sizeof(uint16_t));

	if (!file)
		throw std::runtime_error("读取数据失败: " + rawPath.string());

	int step = 8;
	uint32_t dataZMipWidth = width / step; // 如果高度图是2049，这里/8会自动抹除余数
	uint32_t dataZMipHeight = height / step;
	std::vector<MinMaxZMap> dataZMip0(dataZMipWidth * dataZMipHeight);

	for (int y = 0; y + step < height; y += step) // width height -1 防止边界溢出
	{
		for (int x = 0; x + step < width; x += step)
		{
			float minZ = std::numeric_limits<float>::max();
			float maxZ = std::numeric_limits<float>::lowest();

			for (int j = 0; j <= step; j++)
			{
				for (int i = 0; i <= step; i++) // mip0的循环 使用<=
				{
					int yy = y + j; if (yy >= height) yy = height - 1; // 防止越界
					int xx = x + i; if (xx >= width)  xx = width - 1;  

					int index = yy * width + xx;
					uint16_t value = rawData[index];

					float normalizedValue = static_cast<float>(value) / 65535.0f;
					float tempHeight = (float)m_minZ + normalizedValue * (float)(m_maxZ - m_minZ);

					minZ = std::min(minZ, tempHeight);
					maxZ = std::max(maxZ, tempHeight);
				}
			}

			dataZMip0[(y / step) * dataZMipWidth + (x / step)].minVal = minZ;
			dataZMip0[(y / step) * dataZMipWidth + (x / step)].maxVal = maxZ;
		}
	}

	int mipStep = 2;
	uint32_t prevW = dataZMipWidth;
	uint32_t prevH = dataZMipHeight;
	std::vector<std::vector<MinMaxZMap>> dataZMip1To5(5);
	for (int mip = 0; mip < 5; mip++)
	{
		uint32_t currW = prevW / mipStep;
		uint32_t currH = prevH / mipStep;
		dataZMip1To5[mip].resize(currW * currH);

		for (int y = 0; y < prevH; y += mipStep)
		{
			for (int x = 0; x < prevW; x += mipStep)
			{
				float minZ = std::numeric_limits<float>::max();
				float maxZ = std::numeric_limits<float>::lowest();

				for (int j = 0; j < mipStep; j++)
				{
					for (int i = 0; i < mipStep; i++)
					{
						int index = (y + j) * prevW + x + i;
						auto data = mip == 0 ? dataZMip0[index] : dataZMip1To5[mip - 1][index];

						minZ = std::min(minZ, data.minVal);
						maxZ = std::max(maxZ, data.maxVal);
					}
				}

				dataZMip1To5[mip][(y / mipStep) * currW + (x / mipStep)].minVal = minZ;
				dataZMip1To5[mip][(y / mipStep) * currW + (x / mipStep)].maxVal = maxZ;
			}
		}

		prevH = currH;
		prevW = currW;
	}

	std::shared_ptr<ScratchImage> pImage = std::make_shared<ScratchImage>();
	HRESULT hr = pImage->Initialize2D(DXGI_FORMAT_R32G32_FLOAT, dataZMipWidth, dataZMipHeight, 1, 6);

	auto fillLevel = [&](uint32_t mip, const std::vector<MinMaxZMap>& src)
		{
			const Image* dst = pImage->GetImage(mip, 0, 0);
			MinMaxZMap* pDst = reinterpret_cast<MinMaxZMap*>(dst->pixels);
			for (uint32_t i = 0; i < src.size(); ++i)
			{
				pDst[i] = src[i];
			}
		};

	fillLevel(0, dataZMip0);
	for (int i = 0; i < 5; ++i) fillLevel(i + 1, dataZMip1To5[i]);

	m_minMaxZMapPath = m_path;
	m_minMaxZMapPath.replace_filename(m_path.stem().string() + "_MinMaxZ");
	m_minMaxZMapPath.replace_extension(".dds");

	hr = SaveToDDSFile(pImage->GetImages(), pImage->GetImageCount(), pImage->GetMetadata(), DDS_FLAGS_NONE, m_minMaxZMapPath.wstring().c_str());
}

void NXTerrainLayer::GeneratePatchConeMap()
{
}
