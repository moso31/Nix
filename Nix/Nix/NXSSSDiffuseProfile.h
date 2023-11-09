#pragma once
#include "NXObject.h"
#include "NXSerializable.h"

class NXSSSDiffuseProfile : public NXObject, public NXSerializable
{
public:
	NXSSSDiffuseProfile() {}
	~NXSSSDiffuseProfile() {}

	virtual void Serialize() override;
	virtual void Deserialize() override;

	void SetFilePath(const std::filesystem::path& path) { m_filePath = path; }

private:
	std::filesystem::path m_filePath;

	Vector3 m_scatter = Vector3(1.0f);
	float m_scatterStrength = 1.0f;
	Vector3 m_transmit = Vector3(1.0f);
	float m_transmitStrength = 1.0f;
};
