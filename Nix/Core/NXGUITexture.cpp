#include "BaseDefs/DearImGui.h"

#include "NXGUITexture.h"
#include "NXGUICommon.h"
#include "NXResourceManager.h"
#include "NXPBRMaterial.h"
#include "NXTexture.h"
#include "NXGUIInspector.h"
#include "NXAllocatorManager.h"
#include "DirectXTex.h"

using namespace DirectX;

NXGUITexture::NXGUITexture()
{
}

void NXGUITexture::Render()
{
	ImGui::Text("Texture");

	if (m_pTexImage.IsNull())
	{ 
		return;
	}

	if (m_path.extension() == ".raw")
	{
		Render_RawTexture();
	}
	else
	{
		Render_Texture();
	}
}

void NXGUITexture::Release()
{
}

void NXGUITexture::SetImage(const std::filesystem::path& path)
{
	if (m_pTexImage.IsValid() && path == m_pTexImage->GetFilePath())
		return;

	m_path = path;
	m_pTexImage = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("NXGUITexture Preview Image", path);
	m_texData = m_pTexImage->GetSerializationData();
}

void NXGUITexture::Render_Texture()
{
	float fTexSize = ImGui::GetContentRegionAvail().x * 0.7f;
	NXShVisDescHeap->PushFluid(m_pTexImage->GetSRV());
	auto& srvHandle = NXShVisDescHeap->Submit();
	const ImTextureID& ImTexID = (ImTextureID)srvHandle.ptr;
	ImGui::Image(ImTexID, ImVec2(fTexSize, fTexSize));

	ImGui::Checkbox("Generate mip map##Texture", &m_texData.m_bGenerateMipMap);
	ImGui::Checkbox("Invert normal Y##Texture", &m_texData.m_bInvertNormalY);

	const char* strTextureTypes[] = { "Raw", "sRGB", "Linear", "Normal Map" };
	int nTexType = (int)m_texData.m_textureType;
	if (ImGui::Combo("Texture type##Texture", &nTexType, strTextureTypes, IM_ARRAYSIZE(strTextureTypes)))
	{
		m_texData.m_textureType = (NXTextureMode)(nTexType);
	}

	if (ImGui::Button("Apply##Texture"))
	{
		// 根据GUI参数更新序列化相关数据
		m_pTexImage->SetSerializationData(m_texData);

		// 序列化，保存成n0文件
		m_pTexImage->Serialize();

		// 进行异步重载
		m_pTexImage->MarkReload(m_pTexImage->GetFilePath());
	}
}

void NXGUITexture::Render_RawTexture()
{
	float fTexSize = ImGui::GetContentRegionAvail().x * 0.7f;
	NXShVisDescHeap->PushFluid(m_pTexImage->GetSRV());
	auto& srvHandle = NXShVisDescHeap->Submit();
	const ImTextureID& ImTexID = (ImTextureID)srvHandle.ptr;
	ImGui::Image(ImTexID, ImVec2(fTexSize, fTexSize));

	static int value[2] = { m_texData.m_rawWidth, m_texData.m_rawHeight };
	if (ImGui::InputInt2("Texture Size", value))
	{
		m_texData.m_rawWidth = value[0];
		m_texData.m_rawHeight = value[1];
	}

	if (ImGui::Button("Apply##Texture"))
	{
		// 根据GUI参数更新序列化相关数据
		m_pTexImage->SetSerializationData(m_texData);

		// 序列化，保存成n0文件
		m_pTexImage->Serialize();

		// 进行异步重载
		m_pTexImage->MarkReload(m_pTexImage->GetFilePath());
	}

	if (ImGui::Button("Bake normal map..."))
	{
		ImGui::OpenPopup("##BakePopup");
	}

	Render_BakePopup();
}

void NXGUITexture::Render_BakePopup()
{
	if (ImGui::BeginPopup("##BakePopup"))
	{
		ImGui::PushID("##BakePopup");
		ImGui::DragFloat2("WorldSize", m_terrainNormalMapBakeData.worldSize);
		ImGui::DragFloat2("HeightRange", m_terrainNormalMapBakeData.heightRange);
		if (ImGui::Button("Bake!"))
		{
			SaveNormalMap();
		}
		ImGui::PopID();
		ImGui::EndPopup();
	}
}

void NXGUITexture::SaveNormalMap()
{
	std::filesystem::path rawPath = m_pTexImage->GetFilePath();
	int width = m_texData.m_rawWidth;
	int height = m_texData.m_rawHeight;

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

	// 读取高度图raw纹理
	uint32_t bytePerPixel = sizeof(uint16_t);
	float normValue = (float)(1 << 16);

	auto fmt = DXGI_FORMAT_R16_UNORM; 
	std::shared_ptr<ScratchImage> pImage = std::make_shared<ScratchImage>();
	pImage->Initialize2D(fmt, width, height, 1, 1);

	const Image& image = *pImage->GetImage(0, 0, 0);
	uint16_t* p = reinterpret_cast<uint16_t*>(image.pixels);
	memcpy(p, rawData.data(), width * height * bytePerPixel);

	// 基于中心和周围四点坐标计算法线值
	std::vector<Vector3> normalMapData(width * height);
	for (uint32_t i = 0; i < width; ++i)
	{
		for (uint32_t j = 0; j < height; ++j)
		{
			uint32_t iL = (i == 0) ? i : i - 1;
			uint32_t iR = (i == width - 1) ? i : i + 1;
			uint32_t jU = (j == 0) ? j : j - 1;
			uint32_t jD = (j == height - 1) ? j : j + 1;

			float hL = (static_cast<float>(rawData[j * width + iL]) / normValue) * (m_terrainNormalMapBakeData.heightRange[1] - m_terrainNormalMapBakeData.heightRange[0]) + m_terrainNormalMapBakeData.heightRange[0];
			float hR = (static_cast<float>(rawData[j * width + iR]) / normValue) * (m_terrainNormalMapBakeData.heightRange[1] - m_terrainNormalMapBakeData.heightRange[0]) + m_terrainNormalMapBakeData.heightRange[0];
			float h  = (static_cast<float>(rawData[j * width + i])  / normValue) * (m_terrainNormalMapBakeData.heightRange[1] - m_terrainNormalMapBakeData.heightRange[0]) + m_terrainNormalMapBakeData.heightRange[0];
			float hU = (static_cast<float>(rawData[jU * width + i]) / normValue) * (m_terrainNormalMapBakeData.heightRange[1] - m_terrainNormalMapBakeData.heightRange[0]) + m_terrainNormalMapBakeData.heightRange[0];
			float hD = (static_cast<float>(rawData[jD * width + i]) / normValue) * (m_terrainNormalMapBakeData.heightRange[1] - m_terrainNormalMapBakeData.heightRange[0]) + m_terrainNormalMapBakeData.heightRange[0];

			Vector3 posL((float)iL / (float)width * m_terrainNormalMapBakeData.worldSize[0], hL, (float)j  / (float)width * m_terrainNormalMapBakeData.worldSize[1]);
			Vector3 posR((float)iR / (float)width * m_terrainNormalMapBakeData.worldSize[0], hR, (float)j  / (float)width * m_terrainNormalMapBakeData.worldSize[1]);
			//Vector3 posC((float)i  / (float)width * m_terrainNormalMapBakeData.worldSize[0], h,  (float)j  / (float)width * m_terrainNormalMapBakeData.worldSize[1]);
			Vector3 posU((float)i  / (float)width * m_terrainNormalMapBakeData.worldSize[0], hU, (float)jU / (float)width * m_terrainNormalMapBakeData.worldSize[1]);
			Vector3 posD((float)i  / (float)width * m_terrainNormalMapBakeData.worldSize[0], hD, (float)jD / (float)width * m_terrainNormalMapBakeData.worldSize[1]);

			Vector3 vecLR = posR - posL;
			Vector3 vecUD = posD - posU;

			Vector3 vec = vecUD.Cross(vecLR);
			vec.Normalize();

			normalMapData[j * width + i] = vec;
		}
	}
	pImage.reset();

	// 用获得的法线数据生成 地形法线纹理
	fmt = DXGI_FORMAT_R10G10B10A2_UNORM;
	pImage = std::make_shared<ScratchImage>();
	pImage->Initialize2D(fmt, width, height, 1, 1);
	const Image oImage = *pImage->GetImage(0, 0, 0);

	for (uint32_t y = 0; y < height; ++y)
	{
		uint32_t* row = reinterpret_cast<uint32_t*>(oImage.pixels + y * oImage.rowPitch);
		for (uint32_t x = 0; x < width; ++x)
		{
			const Vector3& n = normalMapData[y * width + x];

			// 映射 [-1,1] 到 [0,1023]
			uint32_t r = static_cast<uint32_t>((n.x * 0.5f + 0.5f) * 1023.0f);
			uint32_t g = static_cast<uint32_t>((n.y * 0.5f + 0.5f) * 1023.0f);
			uint32_t b = static_cast<uint32_t>((n.z * 0.5f + 0.5f) * 1023.0f);
			uint32_t a = 3; // 2-bit alpha，设置为最大值

			row[x] = (a << 30) | (b << 20) | (g << 10) | (r << 0);
		}
	}

	std::filesystem::path strNormalMap = rawPath.parent_path() / (rawPath.stem().string() + "_normal.dds");
	HRESULT hr = SaveToDDSFile(oImage, DDS_FLAGS_NONE, strNormalMap.wstring().c_str());

	auto pNewTex = NXResourceManager::GetInstance()->GetTextureManager()->CreateTexture2D("Terrain Normal Map", strNormalMap);

	pNewTex->SetSerializationData(m_texData);
	pNewTex->Serialize();
	pNewTex->MarkReload(strNormalMap);
}
