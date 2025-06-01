#pragma once
#include "NXGraphicPass.h"
#include "ShaderStructures.h"
#include "Ntr.h"

class NXScene;
class NXBRDFLut;
class NXTexture2D;
class NXRenderTarget;
class NXDeferredRenderer : public NXGraphicPass
{
public:
	NXDeferredRenderer(NXScene* pScene);
	virtual ~NXDeferredRenderer();

	virtual void SetupInternal() override;

private:
	NXScene* m_pScene;
};
