#pragma once
#include "Header.h"

class NXGUIRAMTest
{
public:
	explicit NXGUIRAMTest() = default;
	NXGUIRAMTest(NXScene* pScene);
	~NXGUIRAMTest() {}

	void Init();
	void RenderBoxes();
	void Render();

	void Release();

private:
	std::vector<NXTexture2D*> m_pTextures;
	NXScene* m_pScene;
	NXTestMaterial* m_pTestMat;
	std::vector<NXPrimitive*> m_pTestBoxes;
};
