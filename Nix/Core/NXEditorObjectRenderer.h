#pragma once
#include "NXGraphicPass.h"
#include "ShaderStructures.h"

class NXScene;
class NXEditorObjectRenderer : public NXGraphicPass
{
public:
	NXEditorObjectRenderer(NXScene* pScene);
	virtual ~NXEditorObjectRenderer();

	virtual void SetupInternal() override;
	virtual void Render() override;

	void Release();

	bool GetEnable() { return true; }
	void SetEnable(bool value) { m_bEnable = value; }

private:
	bool m_bEnable;
	NXScene* m_pScene;
};
