#pragma once
#include <string>
#include "BaseDefs/NixCore.h"
#include "BaseDefs/Math.h"
#include "Ntr.h"

class NXTexture2D;
class NXShaderVisibleDescriptorHeap;
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
	NXGUIView() : m_viewMode(ViewMode::Auto) {}
	virtual ~NXGUIView() {}

	void SetViewRT(Ntr<NXTexture2D> pTexture2D);
	Ntr<NXTexture2D> GetViewRT() { return m_pViewRT; }

	void Render(ccmem::DescriptorAllocator<true>* pImguiHeap);

private:
	const std::string GetViewModeString() const;

private:
	Ntr<NXTexture2D> m_pViewRT;
	
	Vector2 m_viewRTSize;

	ViewMode m_viewMode;
};
