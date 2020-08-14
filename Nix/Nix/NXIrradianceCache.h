#pragma once
#include "Header.h"
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
	NXIrradianceCache() {}
	~NXIrradianceCache() {}

	void SetPhotonMaps(const shared_ptr<NXPhotonMap>& pPhotonMap) { m_pPhotonMap = pPhotonMap; }
	Vector3 Irradiance(const Ray& ray, const shared_ptr<NXScene>& pScene, int depth);

private:
	shared_ptr<NXPhotonMap> m_pPhotonMap;
	vector<NXIrradianceCacheInfo> m_cacheInfo;
};