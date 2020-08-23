#pragma once
#include <execution>
#include "ImageGenerator.h"
#include "NXRayTracePass.h"
#include "NXPPMPixel.h"
#include "NXPhoton.h"

class NXRayPassPPMGeneratePixels : public NXRayTracePass
{
public:
	NXRayPassPPMGeneratePixels() {}
	~NXRayPassPPMGeneratePixels() {}

	void Load(const std::shared_ptr<NXScene>& pScene, const XMINT2& imageSize, const XMINT2& tileSize, int eachPixelSamples);
	void Render() override;

	std::shared_ptr<NXPPMPixelGenerator> GetPPMPixelGenerator() { return m_pPPMPixelGenerator; }

private:
	void RenderImageDataParallel(bool useOpenMP);
	void RenderTile(const XMINT2& tileId);

private:
	std::shared_ptr<NXScene> m_pScene;
	std::shared_ptr<NXPPMPixelGenerator> m_pPPMPixelGenerator;

	XMINT2 m_imageSize;			// ��ǰPass�ķֱ��ʴ�С
	XMINT2 m_tileSize;			// �����Ҫ��tile���̼߳���
	XMINT2 m_tileCount;			// tile����
	int m_eachPixelSamples;		// ÿ�����ض������� 

	std::mutex m_mutexPixelInfo;	// ��ʹ�ö��߳����PixelInfo����ʱ��Ҫʹ�û�������
};

class NXRayPassPPM : public NXRayTracePass
{
public:
	NXRayPassPPM() {}
	~NXRayPassPPM() {}

	void Load(const std::shared_ptr<NXScene>& pScene, const std::shared_ptr<NXPPMPixelGenerator>& pPPMPixelGenerator, const XMINT2& imageSize, int nPhotonsAtOnce);
	void GeneratePhotonMap();
	void Render() override;

	void Release();

private:
	void RenderOneSample(ImageBMPData* pImageData, PPMPixel& pixel);

private:
	std::shared_ptr<NXScene> m_pScene;
	std::shared_ptr<NXPPMPixelGenerator> m_pPPMPixelGenerator;
	std::shared_ptr<NXPhotonMap> m_pPhotonMap;
	ImageBMPData* m_pImageData;	// ����ͼ�������

	XMINT2 m_imageSize;			// ��ǰPass�ķֱ��ʴ�С

	int m_accumulatePhotons;	// �ۻ���������
	int m_nPhotonsAtOnce;	// һ��pass���������ɵĹ�������
};