#pragma once
#include "Header.h"

class NXRDistribution
{
public:
	NXRDistribution() = default;
	virtual ~NXRDistribution() {};

	virtual float D(const Vector3& wh) = 0;
	float G(const Vector3& wo, const Vector3& wi);
	virtual Vector3 Sample_wh(const Vector3& wo) = 0;
	float Pdf(const Vector3 &wh);

protected:
	virtual float lambda(const Vector3& w) = 0;
};

class NXRDistributionBeckmann : public NXRDistribution
{
public:
	static float RoughnessToAlpha(float roughness) {
		roughness = max(roughness, (float)1e-3);
		float x = logf(roughness);
		return 1.62142f + 0.819955f * x + 0.1734f * x * x +
			0.0171201f * x * x * x + 0.000640711f * x * x * x * x;
	}

	NXRDistributionBeckmann(float alpha) : alpha(alpha) {}
	~NXRDistributionBeckmann() {}

	float D(const Vector3& wh) override;
	Vector3 Sample_wh(const Vector3& wo) override;

protected:
	float lambda(const Vector3& w) override;

private:
	float alpha;
};

class NXRDistributionGGX : public NXRDistribution
{
public:
};

class NXRDistributionPhong : public NXRDistribution
{
public:
};