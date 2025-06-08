#pragma once
#include "NXGraphicPass.h"
#include "NXConstantBuffer.h"

struct CBufferColorMapping
{
	Vector4 param0; // x: enable
};

class NXColorMappingRenderer : public NXGraphicPass
{
public:
	NXColorMappingRenderer();
	virtual ~NXColorMappingRenderer();

	virtual void SetupInternal() override;

	void Release();

private:
};
