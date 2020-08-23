#pragma once
#include "NXBSDF.h"

struct PPMPixel
{
	PPMPixel() {}
	~PPMPixel() {}

	XMINT2 pixel;	// 像素坐标
	float pixelWeight;	// 权重
	Vector3 position;
	Vector3 normal;
	std::shared_ptr<NXBSDF> BSDF;
	Vector3 direction;
	float radius2;
	UINT photons;
	Vector3 flux;
	Vector3 Lemit;	// 自发光（如果有的话）
};

class NXPPMPixelGenerator
{
public:
	NXPPMPixelGenerator() {}
	~NXPPMPixelGenerator() {}

	bool CalculatePixelInfo(const Ray& ray, const std::shared_ptr<NXScene>& pScene, const XMINT2& pixel, float pixelWeight, std::shared_ptr<PPMPixel>& oInfo, int depth = 0);
	void Resize(const XMINT2& ImageSize, int eachPixelSamples);
	void AddPixelInfo(const std::shared_ptr<PPMPixel>& info);
	std::vector<std::shared_ptr<PPMPixel>> GetPixelInfoList() { return m_pixelInfoList; }

private:
	std::vector<std::shared_ptr<PPMPixel>> m_pixelInfoList;
	XMINT2 m_ImageSize;
};
