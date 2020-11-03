#pragma once
#include "NXIntersection.h"
#include "NXPhoton.h"

struct NXIrradianceCacheInfo
{
	Vector3 position;
	Vector3 normal;
	float harmonicDistance;
	Vector3 irradiance;
};

class NXScene;
class NXIrradianceCache
{
public:
	NXIrradianceCache();
	~NXIrradianceCache() {}

	void AddCache(const NXIrradianceCacheInfo& cache) { m_caches.push_back(cache); };
	size_t GetCacheSize() { return m_caches.size(); }
	void SetPhotonMaps(NXPhotonMap* pPhotonMap) { m_pPhotonMap = pPhotonMap; }

	void PreIrradiance(const Ray& ray, NXScene* pScene, int depth);
	Vector3 Irradiance(const Ray& ray, NXScene* pScene, int depth);
	bool FindEstimateCaches(const Vector3& position, const Vector3& normal, Vector3& oEstimateIrradiance);
	bool CalculateOneCache(NXScene* pScene, const NXHit& hitInfo, int sampleTheta, int samplePhi, Vector3& oIrradiance, NXIrradianceCacheInfo& oCacheInfo);

	void Render(NXScene* pScene, const XMINT2& imageSize, std::string outFilePath);

	void Release();

private:
	NXPhotonMap* m_pPhotonMap;
	std::vector<NXIrradianceCacheInfo> m_caches;
	float m_threshold;
};