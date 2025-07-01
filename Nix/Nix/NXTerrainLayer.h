#pragma once
#include <filesystem>
#include "NXSerializable.h"
#include "Ntr.h"
#include "NXTerrainCommon.h"

class NXTexture2D;
class NXTerrainLayer : public NXSerializable
{
public:
	NXTerrainLayer(const std::string& name);
	~NXTerrainLayer() {}

	const std::filesystem::path& GetPath() const { return m_path; }
	void SetPath(const std::filesystem::path& path, bool bForceCreate = false);

	uint32_t GetTerrainWidth() const { return m_terrainWidth; }
	uint32_t GetTerrainHeight() const { return m_terrainHeight; }

	const std::filesystem::path& GetHeightMapPath() const { return m_heightMapPath; }
	void SetHeightMapPath(const std::filesystem::path& heightMapPath);

	Ntr<NXTexture2D> GetHeightMapTexture() const { return m_heightMapTexture; }
	Ntr<NXTexture2D> GetMinMaxZMapTexture() const { return m_minMaxZMapTexture; }
	Ntr<NXTexture2D> GetPatchConeMapTexture() const { return m_patchConeMapTexture; }

	virtual void Serialize() override;
	virtual void Deserialize() override;

	void Release();

	void BakeGPUDrivenData(bool bSave);

private:
	void GenerateMinMaxZMap();
	void GeneratePatchConeMap();

private:
	std::string m_name;

	// ‘› ±–¥À¿
	uint32_t m_terrainWidth = 2048u;
	uint32_t m_terrainHeight = 2048u;
	uint32_t m_minZ = 0;
	uint32_t m_maxZ = 2048;

	std::filesystem::path m_path;
	std::filesystem::path m_heightMapPath;
	std::filesystem::path m_minMaxZMapPath;
	std::filesystem::path m_patchConeMapPath;

	Ntr<NXTexture2D> m_heightMapTexture;
	Ntr<NXTexture2D> m_minMaxZMapTexture;
	Ntr<NXTexture2D> m_patchConeMapTexture;
};
