#pragma once
#include "NXRendererPass.h"

class NXScene;
class NXCamera;
class NXGBufferRenderer : public NXRendererPass
{
private:
	explicit NXGBufferRenderer() = default;
public:
	NXGBufferRenderer(NXScene* pScene);
	virtual ~NXGBufferRenderer();

	virtual void SetupInternal() override {}
	virtual void Render(ID3D12GraphicsCommandList* pCmdList) override;

	void SetCamera(NXCamera* pCamera);

	void Release();

private:
	NXScene* m_pScene;
	NXCamera* m_pCamera;
};
