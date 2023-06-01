#pragma once
#include "Header.h"

extern bool g_isMouseHoverOnView;

class NXFinalRenderer;
// 2023.6.1 ������Ⱦ�ӿ�RT
class NXGUIView
{
public:
	NXGUIView(NXFinalRenderer* pFinalRenderer) : m_pFinalRenderer(pFinalRenderer) {} 
	~NXGUIView() {}

	void Init();
	void Render();

private:
	NXFinalRenderer* m_pFinalRenderer;
};
