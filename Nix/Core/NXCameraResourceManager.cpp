#include "NXCameraResourceManager.h"
#include "NXScene.h"
#include "NXCamera.h"

void NXCameraResourceManager::SetWorkingScene(NXScene* pScene)
{
	m_pWorkingScene = pScene;
}

NXCamera* NXCameraResourceManager::CreateCamera(const std::string& name, const float FovY, const float zNear, const float zFar, const Vector3& eye, const Vector3& at, const Vector3& up, const Vector2& rtSize)
{
	if (m_cameras.find(name) != m_cameras.end())
	{
		return m_cameras[name];
	}

	auto p = new NXCamera(name);
	p->Init(FovY, zNear, zFar, eye, at, up, rtSize);

	m_pWorkingScene->RegisterCamera(p, true);
	m_cameras[name] = p;
	return p;
}

void NXCameraResourceManager::OnReload()
{
}

void NXCameraResourceManager::Release()
{
}
