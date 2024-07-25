#pragma once

class NXShadowMapRenderer;
class NXGUIShadows
{
public:
	NXGUIShadows(NXShadowMapRenderer* pShadowMap) : m_pShadowMap(pShadowMap) {}
	virtual ~NXGUIShadows() {}

	void Render();

private:
	NXShadowMapRenderer* m_pShadowMap;
};