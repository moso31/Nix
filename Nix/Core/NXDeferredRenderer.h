#pragma once
#include "NXRendererPass.h"
#include "ShaderStructures.h"
#include "Ntr.h"

class NXScene;
class NXBRDFLut;
class NXTexture2D;
class NXRenderTarget;
class NXDeferredRenderer : public NXRendererPass
{
public:
	NXDeferredRenderer(NXScene* pScene);
	virtual ~NXDeferredRenderer();

	virtual void SetupInternal() override {}

private:
	NXScene* m_pScene;
};
