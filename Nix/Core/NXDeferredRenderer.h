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
	~NXDeferredRenderer();

	void Init();
	void Render(ID3D12GraphicsCommandList* pCmdList);

	void Release();

private:
	NXBRDFLut* m_pBRDFLut;
	NXScene* m_pScene;
};
