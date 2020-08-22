#pragma once
#include <execution>
#include "NXRayTracePass.h"
#include "NXDirectIntegrator.h"
#include "ImageGenerator.h"

class NXRayTracePassDirect : public NXRayTracePass
{
public:
	NXRayTracePassDirect() {}
	~NXRayTracePassDirect() {}

	void Load(const std::shared_ptr<NXScene>& pScene, XMINT2 imageSize, XMINT2 tileSize, int eachPixelSamples);
	void Render() override;

private:
	void RenderImageDataParallel(ImageBMPData* pImageData, bool useOpenMP);
	void RenderTile(ImageBMPData* pImageData, const XMINT2& tileId);

private:
	std::shared_ptr<NXScene> m_pScene;

	XMINT2 m_imageSize;			// ��ǰPass�ķֱ��ʴ�С
	XMINT2 m_tileSize;			// �����Ҫ��tile���̼߳���
	int m_eachPixelSamples;		// ÿ�����ض������� 

	// ���ټ���
	Vector2 m_imageSizeInv;	
	XMINT2 m_tileCount;

	std::shared_ptr<NXDirectIntegrator> m_pIntegrator;
	std::vector<ImageBMPData> pImageData;

	// ������
	int m_progress;
};