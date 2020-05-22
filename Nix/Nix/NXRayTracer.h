#pragma once
#include "NXInstance.h"
#include "NXIntegrator.h"
#include "HBVH.h"

// ������Ⱦ��ͼ����Ϣ
struct NXRenderImageInfo
{
	// ͼ��ֱ���
	XMINT2 ImageSize;

	// ÿ�������е���������
	int EachPixelSamples;

	// ���ͼ��·��
	string outPath;
};

class NXRayTracer : public NXInstance<NXRayTracer>
{
public:
	NXRayTracer();
	~NXRayTracer() {}

	void MakeImage(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator, const NXRenderImageInfo& ImageInfo);
	void CenterRayTest(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pMainCamera, const shared_ptr<NXIntegrator>& pIntegrator);

	void Release();
};
