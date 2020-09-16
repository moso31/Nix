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
	void SetPhotonMaps(const std::shared_ptr<NXPhotonMap>& pPhotonMap) { m_pPhotonMap = pPhotonMap; }

	void PreIrradiance(const Ray& ray, const std::shared_ptr<NXScene>& pScene, int depth);
	Vector3 Irradiance(const Ray& ray, const std::shared_ptr<NXScene>& pScene, int depth);
	bool FindEstimateCaches(const Vector3& position, const Vector3& normal, Vector3& oEstimateIrradiance);
	Vector3 CalculateOneCache(const std::shared_ptr<NXScene>& pScene, const NXHit& hitInfo, int sampleTheta, int samplePhi, NXIrradianceCacheInfo& oCacheInfo);

	void Render(const std::shared_ptr<NXScene>& pScene, const XMINT2& imageSize, std::string outFilePath);

private:
	std::shared_ptr<NXPhotonMap> m_pPhotonMap;
	std::vector<NXIrradianceCacheInfo> m_caches;
	float m_threshold;
};