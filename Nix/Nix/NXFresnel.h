#pragma once
#include "Header.h"

class NXFresnel
{
public:
	NXFresnel();
	~NXFresnel();

	virtual Vector3 FresnelReflectance(float cosThetaI) = 0;

private:
	
};

class NXFresnelDielectric : public NXFresnel
{
public:
	NXFresnelDielectric(float etaI, float etaT) : etaI(etaI), etaT(etaT) {}
	~NXFresnelDielectric() {}

	Vector3 FresnelReflectance(float cosThetaI);

private:
	float etaI, etaT;
};

class NXFresnelConductor : public NXFresnel
{
public:
	NXFresnelConductor(const Vector3& etaI, const Vector3& etaT, const Vector3& kT) : etaI(etaI), etaT(etaT), kT(kT) {}
	~NXFresnelConductor() {}

	Vector3 FresnelReflectance(float cosThetaI);

private:
	Vector3 etaI, etaT, kT;
};