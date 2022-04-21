#pragma once
#include "Header.h"

enum HBVHSplitMode
{
	SplitPosition,
	SplitCount,
	SAH,
	HLBVH
};

struct HBVHPrimitiveInfo
{
	HBVHPrimitiveInfo() = default;
	AABB aabb;
	int index;
};

struct HBVHBucketInfo
{
	HBVHBucketInfo() = default;
	AABB aabb;
	int nPrimitive = 0;
};

struct HBVHTreeNode
{
	HBVHTreeNode() : 
		index(-1),
		offset(-1)
	{
		child[0] = nullptr;
		child[1] = nullptr;
		aabb.Extents = Vector3(0.0f);
	}

	~HBVHTreeNode()	{}

	HBVHTreeNode* child[2];
	AABB aabb;
	int index;
	int offset;
};

struct HBVHMortonPrimitiveInfo
{
	HBVHMortonPrimitiveInfo() = default;
	int mortonCode;
	int index;
	AABB aabb;
};

struct HBVHTreeletInfo
{
	HBVHTreeletInfo() = default;
	int startIndex;
	int nPrimitive;
	HBVHTreeNode* node;
};

class NXHit;
class HBVHTree
{
public:
	HBVHTree(NXScene* scene);
	~HBVHTree() {}

	// ���ݳ�����Ϣ�����ɹ���BVH������Ҫ����Ϣ��
	void BuildTreesWithScene(HBVHSplitMode mode);

	// BVH��ײ��⡣
	// ���������������ߣ����SurfaceInteraction�Ͷ����������hitIndex��
	void Intersect(const Ray& worldRay, NXHit& outHitInfo, float tMax = FLT_MAX);

	void Release();

private:
	void BuildTree(HBVHTreeNode* node, int stIndex, int edIndex, HBVHSplitMode mode);
	void RecursiveIntersect(HBVHTreeNode* node, const Ray& worldRay, NXHit& outHitInfo, float& out_tHit);

	// ����HLBVH����Ҫ�õ���treelet��һ�ι���һ����
	HBVHTreeNode* BuildTreelet(int stIndex, int edIndex, int bitIndex);

	// �����ϲ�������
	void BuildUpperTree(HBVHTreeNode*& node, int stIndex, int edIndex);

	void ReleaseTreeNode(HBVHTreeNode* node);

private:
	const int SPLIT_COST = 4;

	HBVHTreeNode* root;
	NXScene* m_pScene;
	std::vector<HBVHPrimitiveInfo> m_primitiveInfo;
	std::vector<HBVHMortonPrimitiveInfo> m_mortonPrimitiveInfo;
	std::vector<HBVHTreeletInfo> m_treeletInfo;

	HBVHSplitMode m_buildMode;
};
