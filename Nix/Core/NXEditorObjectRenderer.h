#pragma once
#include "NXRendererPass.h"
#include "ShaderStructures.h"

class NXScene;
class NXEditorObjectRenderer : public NXRendererPass
{
public:
	NXEditorObjectRenderer(NXScene* pScene);
	virtual ~NXEditorObjectRenderer();

	virtual void SetupInternal() override;
	virtual void Render(ID3D12GraphicsCommandList* pCmdList) override;

	void Release();

	bool GetEnable() { return true; }
	void SetEnable(bool value) { m_bEnable = value; }

private:
	bool m_bEnable;
	NXScene* m_pScene;
};
