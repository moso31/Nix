#pragma once

class NXScene;
class NXCamera;
class NXGUICamera
{
public:
	NXGUICamera(NXScene* pScene = nullptr);
	~NXGUICamera() {}

	void Render();

private:
	NXScene* m_pCurrentScene;
	NXCamera* m_pCurrentCamera;
};