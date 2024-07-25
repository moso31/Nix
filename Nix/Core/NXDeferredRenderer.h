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
	NXDeferredRenderer(NXScene* pScene, NXBRDFLut* pBRDFLut);
	virtual ~NXDeferredRenderer();

	void Init();

private:
	NXBRDFLut* m_pBRDFLut;
	NXScene* m_pScene;
};
