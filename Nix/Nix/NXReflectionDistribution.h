#pragma once
#include "Header.h"

class NXRMicrofacetDistribution
{
public:
	NXRMicrofacetDistribution() = default;
	~NXRMicrofacetDistribution() {};

	virtual float D(const Vector3& wh) = 0;
	float G(const Vector3& wo, const Vector3& wi);
	virtual Vector3 Sample_wh(const Vector3& wo) = 0;
	float Pdf(const Vector3 &wh);

protected:
	virtual float lambda(const Vector3& w) = 0;
};

class NXRDistributionBeckmann : public NXRMicrofacetDistribution
{
public:
	float D(const Vector3& wh) override;
	Vector3 Sample_wh(const Vector3& wo) override;

protected:
	float lambda(const Vector3& w) override;

private:
	float alpha;
};

class NXRDistributionGGX : public NXRMicrofacetDistribution
{
public:
};

class NXRDistributionPhong : public NXRMicrofacetDistribution
{
public:
};