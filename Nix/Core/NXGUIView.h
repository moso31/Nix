#pragma once
#include "Header.h"

extern bool g_isMouseHoverOnView;

class NXFinalRenderer;
// 2023.6.1 用于渲染视口RT
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
