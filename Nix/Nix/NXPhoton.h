#pragma once
#include "Header.h"
#include <queue>

#define priority_quque_NXPhoton priority_queue<NXPhoton*, vector<NXPhoton*>, function<bool(NXPhoton* photonA, NXPhoton* photonB)>>

enum LocateFilter
{
	// 按何种方法统计kNN近邻点。
	Disk,
	Sphere
};

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

class NXKdTree;
class NXPhotonMap
{
public:
	NXPhotonMap(int numPhotons);
	~NXPhotonMap() {}

	void Generate(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pCamera, PhotonMapType photonMapType);

	int GetPhotonCount() { return m_numPhotons; }
	void GetNearest(const Vector3& position, const Vector3& normal, float& out_distSqr, priority_quque_NXPhoton& out_nearestPhotons, int maxLimit = 500, float range = 0.1f, LocateFilter locateFilter = Sphere);

private:
	void GenerateCausticMap(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pCamera);
	void GenerateGlobalMap(const shared_ptr<NXScene>& pScene, const shared_ptr<NXCamera>& pCamera);

private:
	shared_ptr<NXKdTree> m_pKdTree;
	int m_numPhotons;
};
