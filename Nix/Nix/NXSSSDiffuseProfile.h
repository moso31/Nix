#pragma once
#include "NXObject.h"
#include "NXSerializable.h"

class NXSSSDiffuseProfile : public NXObject, public NXSerializable
{
public:
	NXSSSDiffuseProfile() {}
	virtual ~NXSSSDiffuseProfile() {}

	virtual void Serialize() override;
	virtual void Deserialize() override;

	void SetFilePath(const std::filesystem::path& path) { m_filePath = path; }
	const std::filesystem::path& GetFilePath() const { return m_filePath; }

	Vector3 GetScatter() const { return m_scatter; }
	void SetScatter(const Vector3& value) { m_scatter = value; }
	float GetScatterDistance() { return m_scatterDistance; }
	void SetScatterDistance(float value) { m_scatterDistance = value; }

	Vector3 GetTransmit() const { return m_transmit; }
	void SetTransmit(const Vector3& value) { m_transmit = value; }
	float GetTransmitStrength() { return m_transmitStrength; }
	void SetTransmitStrength(float value) { m_transmitStrength = value; }

private:
	std::filesystem::path m_filePath;

	Vector3 m_scatter = Vector3(1.0f);
	float m_scatterDistance = 1.0f;

	Vector3 m_transmit = Vector3(1.0f);
	float m_transmitStrength = 1.0f;
};
