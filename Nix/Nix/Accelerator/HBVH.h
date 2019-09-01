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

	// ���ݳ�����Ϣ�����ɹ���BVH������Ҫ����Ϣ��
	void BuildTreesWithScene(HBVHSplitMode mode);

	// BVH��ײ��⡣
	// ���������������ߣ����SurfaceInteraction�Ͷ����������hitIndex��
	void Intersect(const Ray& worldRay, Vector3& outHitPos, int& outHitIndex, float tMax = FLT_MAX);

private:
	void BuildTree(HBVHTreeNode* node, int stIndex, int edIndex, HBVHSplitMode mode);
	void RecursiveIntersect(HBVHTreeNode* node, const Ray& worldRay, _Out_ Vector3& outHitPos, _Out_ float& out_tResult, _Out_ int& out_hitIndex);

	// ����HLBVH����Ҫ�õ���treelet��һ�ι���һ����
	HBVHTreeNode* BuildTreelet(int stIndex, int edIndex, int bitIndex);

	// �����ϲ�������
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