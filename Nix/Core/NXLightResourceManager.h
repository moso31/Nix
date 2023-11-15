#pragma once
#include "BaseDefs/Math.h"
#include "NXResourceManagerBase.h"

class NXLightResourceManager : public NXResourceManagerBase
{
public:
	NXLightResourceManager() {}
	~NXLightResourceManager() {}

	Ntr<NXPBRDistantLight> CreatePBRDistantLight(const Vector3& direction, const Vector3& color, const float illuminance);
	Ntr<NXPBRPointLight> CreatePBRPointLight(const Vector3& position, const Vector3& color, const float intensity, const float influenceRadius);
	Ntr<NXPBRSpotLight> CreatePBRSpotLight(const Vector3& position, const Vector3& direction, const Vector3& color, const float intensity, const float innerAngle, const float outerAngle, const float influenceRadius);
	NXCubeMap* CreateCubeMap(const std::string name, const std::filesystem::path& filePath);

	void OnReload() override;
	void Release() override;

private:
	std::vector<Ntr<NXPBRLight>>			m_lights;
	std::vector<Ntr<NXPBRDistantLight>>		m_distantLights;
	std::vector<Ntr<NXPBRPointLight>>		m_pointLights;
	std::vector<Ntr<NXPBRSpotLight>>		m_spotLights;
};