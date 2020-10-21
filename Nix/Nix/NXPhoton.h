#pragma once
#include "NXKdTree.h"

struct NXPhoton
{
	Vector3 position;
	Vector3 direction;
	Vector3 power;
	int depth;
};

enum PhotonMapType
{
	Caustic,
	Global,
};

class NXPhotonMap
{
public:
	NXPhotonMap(int numPhotons);
	~NXPhotonMap() {}

	void Generate(const std::shared_ptr<NXScene>& pScene, PhotonMapType photonMapType);

	int GetPhotonCount() const { return m_numPhotons; }
	void GetNearest(const Vector3& position, const Vector3& normal, float& out_distSqr, priority_queue_distance_cartesian<NXPhoton>& out_nearestPhotons, int maxPhotonsLimit, float range2, LocateFilter locateFilter = Sphere);

	void Render(const std::shared_ptr<NXScene>& pScene, const XMINT2& imageSize, std::string outFilePath);

private:
	void GenerateCausticMap(const std::shared_ptr<NXScene>& pScene);
	void GenerateGlobalMap(const std::shared_ptr<NXScene>& pScene);

private:
	std::shared_ptr<NXKdTree<NXPhoton>> m_pKdTree;
	std::vector<NXPhoton> m_pData;
	int m_numPhotons;
};
