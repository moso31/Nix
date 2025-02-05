#pragma once
#include "NXRendererPass.h"
#include "NXConstantBuffer.h"

struct CBufferColorMapping
{
	Vector4 param0; // x: enable
};

class NXColorMappingRenderer : public NXRendererPass
{
public:
	NXColorMappingRenderer();
	virtual ~NXColorMappingRenderer();

	virtual void SetupInternal() override;
	virtual void Render(ID3D12GraphicsCommandList* pCmdList) override;

	bool GetEnable() const { return m_bEnablePostProcessing; }
	void SetEnable(bool value) { m_bEnablePostProcessing = value; }

	void Release();

private:
	bool m_bEnablePostProcessing;

	CBufferColorMapping m_cbData;
	NXConstantBuffer<CBufferColorMapping> m_cb;
};
