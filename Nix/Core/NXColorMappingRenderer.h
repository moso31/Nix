#pragma once
#include "NXRendererPass.h"
#include "NXBuffer.h"

struct CBufferColorMapping
{
	Vector4 param0; // x: enable
};

class NXColorMappingRenderer : public NXRendererPass
{
public:
	NXColorMappingRenderer();
	virtual ~NXColorMappingRenderer();

	void Init();
	void Render(ID3D12GraphicsCommandList* pCmdList);

	bool GetEnable() const { return m_bEnablePostProcessing; }
	void SetEnable(bool value) { m_bEnablePostProcessing = value; }

	void Release();

private:
	bool m_bEnablePostProcessing;

	NXBuffer<CBufferColorMapping> m_cbParams;
};
