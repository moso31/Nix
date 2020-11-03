#pragma once
#include <execution>
#include "NXIntersection.h"
#include "NXScene.h"
#include "ImageGenerator.h"

class NXIntegrator
{
public:
	NXIntegrator() {}
	NXIntegrator(const XMINT2& imageSize) : m_imageSize(imageSize) {}
	~NXIntegrator() {}

	/*
	�Ե��β�������������
	�����DeltaLight��
		����light�ṩ�ķ�����м��㼴�ɵõ�ʵ�ʾ�ȷֵ��
	�������DeltaLight��
		�޷��õ���ȷֵ����ʹ�����ַ���������һ�Σ�
			1.����light����һ�Σ�
			2.����BSDF����һ�Ρ�
		֮�����β����Ľ����ϣ��õ�һ����Ϊ׼ȷ�Ĺ���ֵ��
	*/
	Vector3 DirectEstimate(const Ray& ray, NXScene* pScene, NXPBRLight* pLight, const NXHit& hitInfo);

	Vector3 UniformLightAll(const Ray& ray, NXScene* pScene, const NXHit& hitInfo);
	Vector3 UniformLightOne(const Ray& ray, NXScene* pScene, const NXHit& hitInfo);

	virtual void Render(NXScene* pScene) = 0;

protected:
	XMINT2 m_imageSize;		// ��Ⱦͼ��ֱ���

	// ������
	UINT m_progress;
};

// ������Ⱦ��ͼ����Ϣ
struct NXRenderImageInfo
{
	// ͼ��ֱ���
	XMINT2 ImageSize;

	// ÿ�������е���������
	int EachPixelSamples;

	// ���ͼ��·��
	std::string outPath;
};

class NXSampleIntegrator : public NXIntegrator
{
public:
	NXSampleIntegrator() {}
	NXSampleIntegrator(const XMINT2& imageSize, int eachPixelSamples, std::string outPath);
	~NXSampleIntegrator() {}

	void Render(NXScene* pScene) override;
	virtual Vector3 Radiance(const Ray& ray, NXScene* pScene, int depth) = 0;
	Vector3 CenterRayTest(NXScene* pScene);

private:
	void RenderTile(NXScene* pScene, const XMINT2& tileId, ImageBMPData* oImageData);

public:
	// ��ȾͼƬ���·��
	std::string m_outFilePath;

private:
	XMINT2 m_tileSize;		// ����������ȾTile�Ĵ�С
	UINT m_eachPixelSamples;	// ����������������
};