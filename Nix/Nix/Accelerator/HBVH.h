#pragma once
#if _TODO_
#include <Header.h>

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
	HBVHTreeNode() = default;
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

class HBVHTree
{
public:
	HBVHTree(const shared_ptr<Scene>& scene);
	~HBVHTree() {}

	// 根据场景信息，生成构建BVH树所需要的信息。
	void BuildTreesWithScene(HBVHSplitMode mode);

	// BVH碰撞检测。
	// 给定世界坐标射线，输出SurfaceInteraction和对象的索引号hitIndex，
	void Intersect(const Ray& worldRay, Vector3& outHitPos, int& outHitIndex, float tMax = FLT_MAX);

private:
	void BuildTree(HBVHTreeNode* node, int stIndex, int edIndex, HBVHSplitMode mode);
	void RecursiveIntersect(HBVHTreeNode* node, const Ray& worldRay, _Out_ Vector3& outHitPos, _Out_ float& out_tResult, _Out_ int& out_hitIndex);

	// 构建HLBVH所需要用到的treelet。一次构建一个。
	HBVHTreeNode* BuildTreelet(int stIndex, int edIndex, int bitIndex);

	// 构建上层总树。
	void BuildUpperTree(HBVHTreeNode* node, int stIndex, int edIndex);

private:
	const int SPLIT_COST = 4;

	HBVHTreeNode* root;
	shared_ptr<Scene> m_scene;
	vector<HBVHPrimitiveInfo> m_primitiveInfo;
	vector<HBVHMortonPrimitiveInfo> m_mortonPrimitiveInfo;
	vector<HBVHTreeletInfo> m_treeletInfo;

	HBVHSplitMode m_mode_temp;
};
#endif