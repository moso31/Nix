#include "NXLightResourceManager.h"
#include "NXScene.h"
#include "NXPBRLight.h"
#include "NXCubeMap.h"

Ntr<NXPBRDistantLight> NXLightResourceManager::CreatePBRDistantLight(const Vector3& direction, const Vector3& color, const float illuminance)
{
	Ntr<NXPBRDistantLight> pLight(new NXPBRDistantLight(direction, color, illuminance));
	m_distantLights.push_back(pLight);
	return pLight;
}

Ntr<NXPBRPointLight> NXLightResourceManager::CreatePBRPointLight(const Vector3& position, const Vector3& color, const float intensity, const float influenceRadius)
{
	Ntr<NXPBRPointLight> pLight(new NXPBRPointLight(position, color, intensity, influenceRadius));
	m_pointLights.push_back(pLight);
	return pLight;
}

Ntr<NXPBRSpotLight> NXLightResourceManager::CreatePBRSpotLight(const Vector3& position, const Vector3& direction, const Vector3& color, const float intensity, const float innerAngle, const float outerAngle, const float influenceRadius)
{
	Ntr<NXPBRSpotLight> pLight(new NXPBRSpotLight(position, direction, color, intensity, innerAngle, outerAngle, influenceRadius));
	m_spotLights.push_back(pLight);
	return pLight;
}

NXCubeMap* NXLightResourceManager::CreateCubeMap(const std::string name, const std::filesystem::path& filePath)
{
	auto pCubeMap = new NXCubeMap(m_pWorkingScene);
	pCubeMap->SetName(name);
	pCubeMap->Init(filePath);
	m_pWorkingScene->RegisterCubeMap(pCubeMap);
	return pCubeMap;
}

void NXLightResourceManager::OnReload()
{
}

void NXLightResourceManager::Release()
{
}
