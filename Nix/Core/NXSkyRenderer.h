#pragma once
#include "NXRendererPass.h"

class NXScene;
class NXSkyRenderer : public NXRendererPass
{
public:
	NXSkyRenderer(NXScene* pScene);
	virtual ~NXSkyRenderer();

	virtual void SetupInternal() override;

	void Release() {}

private:
	NXScene* m_pScene;
};
