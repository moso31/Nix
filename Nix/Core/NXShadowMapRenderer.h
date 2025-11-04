#pragma once
#include "BaseDefs/DX12.h"
#include "BaseDefs/Math.h"
#include "BaseDefs/NixCore.h"
#include "ShaderStructures.h"
#include "NXConstantBuffer.h"
#include "NXGraphicPass.h"

struct CBufferShadowMapObject
{
	Matrix view;
	Matrix projection;
};

class NXScene;
class NXRenderableObject;
class NXTexture2DArray;
class NXPBRDistantLight;
class NXShadowMapRenderer : public NXGraphicPass
{
private:
	explicit NXShadowMapRenderer() = default;
public:
	NXShadowMapRenderer(NXScene* pScene);
	virtual ~NXShadowMapRenderer();

	virtual void SetupInternal() override;
	virtual void Render() override;
	void RenderSingleObject(NXRenderableObject* pRenderableObject);

	void Release();

private:
	void RenderCSMPerLight(NXPBRDistantLight* pDirLight);

private:
	NXScene*							m_pScene;
};
