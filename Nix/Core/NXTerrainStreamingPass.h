#pragma once
#include "NXComputePass.h"

class NXTerrainStreamingPass : public NXComputePass
{
public:
	NXTerrainStreamingPass() {}
	virtual ~NXTerrainStreamingPass() {}

	virtual void SetupInternal() override;
};
