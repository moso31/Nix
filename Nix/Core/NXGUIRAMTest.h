#pragma once
#include "Header.h"

class NXGUIRAMTest
{
public:
	explicit NXGUIRAMTest() = default;
	NXGUIRAMTest(NXScene* pScene);
	~NXGUIRAMTest() {}

	void Init();
	void Render();

	void Release();

private:
	void CreateBoxes(bool isFront, const float fOffset, const float fArea, const int nAmount, D3D11_USAGE usage, D3D11_CPU_ACCESS_FLAG cpuAccessFlag, D3D11_BIND_FLAG bindFlag);
	void ClearBoxes();

	NXScene* m_pScene;
	std::vector<NXTexture2D*> m_pTextures;
	std::vector<NXPrimitive*> m_pBoxes;
};
