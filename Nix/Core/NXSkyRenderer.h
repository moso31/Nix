#pragma once
#include "NXRendererPass.h"

class NXScene;
class NXSkyRenderer : public NXRendererPass
{
public:
	NXSkyRenderer(NXScene* pScene);
	~NXSkyRenderer();

	void Init();

	void Release() {}

private:
	NXScene* m_pScene;
};
