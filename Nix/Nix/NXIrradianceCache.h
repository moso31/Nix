#pragma once
#include "NXPhoton.h"
#include "NXIntersection.h"

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

	void AddCache(const NXIrradianceCacheInfo& cache) { m_cacheInfo.push_back(cache); };
	void SetPhotonMaps(const shared_ptr<NXPhotonMap>& pPhotonMap) { m_pPhotonMap = pPhotonMap; }

	void PreIrradiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth);
	Vector3 Irradiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth);
	bool FindEstimateCaches(const Vector3& position, const Vector3& normal, Vector3& oEstimateIrradiance);
	Vector3 CalculateOneCache(const shared_ptr<NXScene>& pScene, const NXHit& hitInfo, int sampleTheta, int samplePhi, NXIrradianceCacheInfo& oCacheInfo);

private:
	shared_ptr<NXPhotonMap> m_pPhotonMap;
	vector<NXIrradianceCacheInfo> m_cacheInfo;
	float m_threshold;
};