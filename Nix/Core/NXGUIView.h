#pragma once
#include "BaseDefs/Math.h"
#include <string>

class NXTexture2D;
// 2023.6.1 用于渲染视口RT
class NXGUIView
{
	enum class ViewMode
	{
		None,
		Auto,
		Custom,
		Fix_1920x1080
	};

public:
	NXGUIView() : m_pViewRT(nullptr), m_viewMode(ViewMode::Auto) {}
	~NXGUIView() {}

	void SetViewRT(NXTexture2D* pTexture2D);
	NXTexture2D* GetViewRT() { return m_pViewRT; }

	void Render();

private:
	const std::string GetViewModeString() const;

private:
	NXTexture2D* m_pViewRT;
	
	Vector2 m_viewRTSize;

	ViewMode m_viewMode;
};
