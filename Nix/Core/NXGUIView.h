#pragma once
#include <string>
#include "BaseDefs/Math.h"
#include "Ntr.h"

class NXTexture2D;
class NXShaderVisibleDescriptorHeap;
// 2023.6.1 ������Ⱦ�ӿ�RT
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
	NXGUIView() : m_viewMode(ViewMode::Auto) {}
	virtual ~NXGUIView() {}

	void SetViewRT(Ntr<NXTexture2D> pTexture2D);
	Ntr<NXTexture2D> GetViewRT() { return m_pViewRT; }

	void Render(NXShaderVisibleDescriptorHeap* pImguiHeap);

private:
	const std::string GetViewModeString() const;

private:
	Ntr<NXTexture2D> m_pViewRT;
	
	Vector2 m_viewRTSize;

	ViewMode m_viewMode;
};
