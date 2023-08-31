#pragma once

class NXShadowMapRenderer;
class NXGUIShadows
{
public:
	NXGUIShadows(NXShadowMapRenderer* pShadowMap) : m_pShadowMap(pShadowMap) {}
	~NXGUIShadows() {}

	void Render();

private:
	NXShadowMapRenderer* m_pShadowMap;
};