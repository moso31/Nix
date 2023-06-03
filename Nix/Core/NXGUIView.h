#pragma once
#include "Header.h"

// 2023.6.1 用于渲染视口RT
class NXGUIView
{
public:
	NXGUIView() : m_pViewRT(nullptr) {}
	~NXGUIView() {}

	void SetViewRT(NXTexture2D* pTexture2D);
	NXTexture2D* GetViewRT() { return m_pViewRT; }

	void Render();

private:
	NXTexture2D* m_pViewRT;
};
