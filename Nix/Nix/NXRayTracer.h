#pragma once
#include "header.h"

class NXIntegrator;

class BSDF
{
public:
	BSDF() {}
	~BSDF() {}

	void ();

private:
	Vector3 p;
	Vector3 wi;
};

// ������Ⱦ��ͼ����Ϣ
struct NXRenderImageInfo
{
	// ͼ��ֱ���
	XMINT2 ImageSize;

	// ÿ�������е���������
	int EachPixelSamples;
};

class NXRayTracer
{
public:
	NXRayTracer();
	~NXRayTracer() {}

	void MakeImage(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo);
	void RayCast(const shared_ptr<NXScene>& pScene, const Ray& rayWorld, const shared_ptr<NXIntegrator>& pIntegrator);

	void Release();

private:
};
