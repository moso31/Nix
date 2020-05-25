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

	Vector3 FresnelReflectance(float cosThetaI) override;

private:
	float etaI, etaT;
};

class NXFresnelConductor : public NXFresnel
{
public:
	NXFresnelConductor(const Vector3& etaI, const Vector3& etaT, const Vector3& kT) : etaI(etaI), etaT(etaT), kT(kT) {}
	~NXFresnelConductor() {}

	Vector3 FresnelReflectance(float cosThetaI) override;

private:
	Vector3 etaI, etaT, kT;
};

class NXFresnelNoOp : public NXFresnel
{
public:
	NXFresnelNoOp() {}
	~NXFresnelNoOp() {}

	Vector3 FresnelReflectance(float cosThetaI) override { return Vector3(1.0f); }
};

class NXFresnelCommon : public NXFresnel
{
public:
	NXFresnelCommon(const Vector3& Specular) : Specular(Specular) {}	// Specular = F0.
	~NXFresnelCommon() {}

	Vector3 FresnelReflectance(float cosThetaH) override;

private:
	Vector3 Specular;
};