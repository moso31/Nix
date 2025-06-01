#pragma once
#include "NXGraphicPass.h"

class NXScene;
class NXSkyRenderer : public NXGraphicPass
{
public:
	NXSkyRenderer(NXScene* pScene);
	virtual ~NXSkyRenderer();

	virtual void SetupInternal() override;

	void Release() {}

private:
	NXScene* m_pScene;
};
