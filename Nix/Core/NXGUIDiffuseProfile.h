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
	void OnSaveClicked();

private:
	Ntr<NXSSSDiffuseProfile> m_pShowProfile;
};
