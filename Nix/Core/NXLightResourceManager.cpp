#include "NXLightResourceManager.h"
#include "NXScene.h"
#include "NXPBRLight.h"
#include "NXCubeMap.h"

void NXLightResourceManager::SetWorkingScene(NXScene* pScene)
{
	m_pWorkingScene = pScene;
}

NXPBRDistantLight* NXLightResourceManager::CreatePBRDistantLight(const Vector3& direction, const Vector3& color, const float illuminance)
{
	auto pLight = new NXPBRDistantLight(direction, color, illuminance);
	m_pWorkingScene->RegisterLight(pLight);
	return pLight;
}

NXPBRPointLight* NXLightResourceManager::CreatePBRPointLight(const Vector3& position, const Vector3& color, const float intensity, const float influenceRadius)
{
	auto pLight = new NXPBRPointLight(position, color, intensity, influenceRadius);
	m_pWorkingScene->RegisterLight(pLight);
	return pLight;
}

NXPBRSpotLight* NXLightResourceManager::CreatePBRSpotLight(const Vector3& position, const Vector3& direction, const Vector3& color, const float intensity, const float innerAngle, const float outerAngle, const float influenceRadius)
{
	auto pLight = new NXPBRSpotLight(position, direction, color, intensity, innerAngle, outerAngle, influenceRadius);
	m_pWorkingScene->RegisterLight(pLight);
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
