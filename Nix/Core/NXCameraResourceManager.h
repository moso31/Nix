#pragma once
#include "BaseDefs/Math.h"
#include "NXResourceManagerBase.h"

class NXCameraResourceManager : public NXResourceManagerBase
{
public:
	NXCameraResourceManager() : m_pWorkingScene(nullptr) {}
	~NXCameraResourceManager() {}

	void SetWorkingScene(NXScene* pScene);

	NXCamera* CreateCamera(const std::string& name, const float FovY, const float zNear, const float zFar, const Vector3& eye, const Vector3& at, const Vector3& up, const Vector2& rtSize);
	NXCamera* GetCamera(const std::string& name) { return m_cameras[name]; }

	void OnReload() override;
	void Release() override;

private:
	NXScene* m_pWorkingScene;
	std::unordered_map<std::string, NXCamera*> m_cameras;
};