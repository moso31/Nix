#pragma once

class NXGPUProfiler;
class NXGUIGPUProfiler
{
public:
	NXGUIGPUProfiler();
	virtual ~NXGUIGPUProfiler() {}

	void Render();

	void SetVisible(bool visible);
	bool IsVisible() const { return m_bShowWindow; }

private:
	bool m_bShowWindow = false;
};
