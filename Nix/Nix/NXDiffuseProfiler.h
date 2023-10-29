#pragma once
#include "BaseDefs/Math.h"
#include "NXSerializable.h"

class NXSSDiffuseProfiler : public NXSerializable
{
public:
	NXSSDiffuseProfiler() {}
	~NXSSDiffuseProfiler() {}

	virtual void Serialize() override;
	virtual void Deserialize() override;

	void SetFilePath(const std::filesystem::path& path) { m_filePath = path; }

private:
	std::filesystem::path m_filePath;

	Vector3 m_scatter;
	float m_scatterStrength;
	Vector3 m_transmit;
	float m_transmitStrength;
};
