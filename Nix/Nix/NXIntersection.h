#pragma once
#include "NXInstance.h"
#include "NXBSDF.h"

class NXHit : public enable_shared_from_this<NXHit>
{
public:
	void ConstructReflectionModel();

	// »÷ÖÐÎïÌå
	shared_ptr<NXPrimitive> primitive;
	Vector3 position;
	float distance;

	NXBSDF bsdf;
};
