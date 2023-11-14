#pragma once
#include <filesystem>
#include "Ntr.h"
#include "BaseDefs/Math.h"

class NXSSSDiffuseProfile;
class NXGUIDiffuseProfile
{
public:
	NXGUIDiffuseProfile();
	~NXGUIDiffuseProfile() {}

	void Render();
	void Release();

	void SetDiffuseProfile(const std::filesystem::path& path);

private:
	Ntr<NXSSSDiffuseProfile> m_pShowProfile;

	Vector3 m_scatter;
	float m_scatterStrength;
	Vector3 m_transmit;
	float m_transmitStrength;
};
