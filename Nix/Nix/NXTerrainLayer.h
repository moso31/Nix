#pragma once
#include <filesystem>
#include "NXSerializable.h"

class NXTerrainLayer : public NXSerializable
{
public:
	NXTerrainLayer(const std::string& name);
	~NXTerrainLayer() {}

	const std::filesystem::path& GetPath() const { return m_path; }
	void SetPath(const std::filesystem::path& path) { m_path = path; }

	const std::filesystem::path& GetHeightMapPath() const { return m_heightMapPath; }
	void SetHeightMapPath(const std::filesystem::path& heightMapPath) { m_heightMapPath = heightMapPath; }

	virtual void Serialize() override;
	virtual void Deserialize() override;

private:
	std::string m_name;
	std::filesystem::path m_path;
	std::filesystem::path m_heightMapPath;
};
