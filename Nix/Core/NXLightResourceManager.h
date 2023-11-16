#pragma once
#include "BaseDefs/Math.h"
#include "NXResourceManagerBase.h"

class NXLightResourceManager : public NXResourceManagerBase
{
public:
	NXLightResourceManager() : m_pWorkingScene(nullptr) {}
	~NXLightResourceManager() {}

	void SetWorkingScene(NXScene* pScene);

	NXPBRDistantLight* CreatePBRDistantLight(const Vector3& direction, const Vector3& color, const float illuminance);
	NXPBRPointLight* CreatePBRPointLight(const Vector3& position, const Vector3& color, const float intensity, const float influenceRadius);
	NXPBRSpotLight* CreatePBRSpotLight(const Vector3& position, const Vector3& direction, const Vector3& color, const float intensity, const float innerAngle, const float outerAngle, const float influenceRadius);
	NXCubeMap* CreateCubeMap(const std::string name, const std::filesystem::path& filePath);

	void OnReload() override;
	void Release() override;

private:
	NXScene* m_pWorkingScene;
};