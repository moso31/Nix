#pragma once
#include "NXGraphicPass.h"

class NXDepthRenderer : public NXGraphicPass
{
public:
	NXDepthRenderer() {}
	virtual ~NXDepthRenderer() {}

	virtual void SetupInternal() override;
};
