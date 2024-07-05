#pragma once
#include "NXRendererPass.h"
#include "NXBuffer.h"
#include "ShaderStructures.h"

class NXScene;
class NXEditorObjectRenderer : public NXRendererPass
{
public:
	NXEditorObjectRenderer(NXScene* pScene);
	~NXEditorObjectRenderer();

	void Init();
	void Render(ID3D12GraphicsCommandList* pCmdList);

	void Release();

	bool GetEnable() { return true; }
	void SetEnable(bool value) { m_bEnable = value; }

private:
	bool m_bEnable;
	NXScene* m_pScene;

	NXBuffer<ConstantBufferVector4>		m_cbParams;
};
