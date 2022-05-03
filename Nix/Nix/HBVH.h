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

	// 根据场景信息，生成构建BVH树所需要的信息。
	void BuildTreesWithScene(HBVHSplitMode mode);

	// BVH碰撞检测。
	// 给定世界坐标射线，输出SurfaceInteraction和对象的索引号hitIndex，
	void Intersect(const Ray& worldRay, NXHit& outHitInfo, float tMax = FLT_MAX);

	void Release();

private:
	void BuildTree(HBVHTreeNode* node, int stIndex, int edIndex, HBVHSplitMode mode);
	void RecursiveIntersect(HBVHTreeNode* node, const Ray& worldRay, NXHit& outHitInfo, float& out_tHit);

	// 构建HLBVH所需要用到的treelet。一次构建一个。
	HBVHTreeNode* BuildTreelet(int stIndex, int edIndex, int bitIndex);

	// 构建上层总树。
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
