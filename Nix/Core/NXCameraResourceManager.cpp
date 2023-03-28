#include "NXCameraResourceManager.h"
#include "NXScene.h"
#include "NXCamera.h"

void NXCameraResourceManager::SetWorkingScene(NXScene* pScene)
{
	m_pWorkingScene = pScene;
}

NXCamera* NXCameraResourceManager::CreateCamera(const std::string name, const float FovY, const float zNear, const float zFar, const Vector3& eye, const Vector3& at, const Vector3& up)
{
	auto p = new NXCamera();
	p->Init(FovY, zNear, zFar, eye, at, up);

	m_pWorkingScene->RegisterCamera(p, true);
	return p;
}

void NXCameraResourceManager::OnReload()
{
}

void NXCameraResourceManager::Release()
{
}
