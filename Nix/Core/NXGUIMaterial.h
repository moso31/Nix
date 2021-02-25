#pragma once
#include "Header.h"
#include "NXGUIFileBrowser.h"
#include "NXScene.h"

class NXGUIMaterial
{
public:
	NXGUIMaterial();
	~NXGUIMaterial() {}

	void SetCurrentScene(NXScene* pScene) { m_pCurrentScene = pScene; }
	void Render();

private:
	// Material GUI ����Ӧ�ĳ���
	NXScene* m_pCurrentScene;
};
