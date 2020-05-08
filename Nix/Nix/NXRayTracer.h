#pragma once
#include "header.h"

class NXIntegrator;

// ������Ⱦ��ͼ����Ϣ
struct NXRenderImageInfo
{
	// ͼ��ֱ���
	XMINT2 ImageSize;

	// ÿ�������е���������
	int SamplerEachPixel;
};

class NXRayTracer
{
public:
	NXRayTracer() {};
	~NXRayTracer() {};

	void MakeImage(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo);

	void Release();

private:
	
};
