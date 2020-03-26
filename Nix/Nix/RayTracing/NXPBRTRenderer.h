#pragma once
#include "Header.h"
#include <random>
#include "NXInstance.h"

struct NXPBRTCamera
{
	Vector3 position;
	Vector3 direction;
	Vector3 up;
	float fFovAngleY;
	short iViewWidth;
	short iViewHeight;
};

struct NXPBRTImage
{
	int iEachPixelSimples;
};

struct NXPhotonMap
{
	int Photons;
};

class NXPBRTRenderer : public NXInstance<NXPBRTRenderer>
{
public:
	NXPBRTRenderer();
	~NXPBRTRenderer() {}

	// 生成光子贴图
	void GeneratePhotonMap(const shared_ptr<NXScene>& pScene, const NXPhotonMap& photonMapInfo);
	void DrawPhotonMapping(const shared_ptr<NXScene>& pScene, const NXPBRTCamera& cameraInfo, const NXPBRTImage& imageInfo);
	Vector3 DrawPhotonMappingPerSample(const shared_ptr<NXScene>& pScene, const Ray& rayWorld);

private:
	default_random_engine m_rng;

	// 将最终输出的图像数据存储到这里。
	vector<Vector3> m_outputImage;
};

