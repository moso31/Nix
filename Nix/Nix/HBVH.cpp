#include "HBVH.h"
#include "NXScene.h"
#include "NXPrimitive.h"

inline int LeftShift3(int x) {
	if (x == (1 << 10)) --x;
	x = (x | (x << 16)) & 0x30000ff;
	x = (x | (x << 8)) & 0x300f00f;
	x = (x | (x << 4)) & 0x30c30c3;
	x = (x | (x << 2)) & 0x9249249;
	return x;
}

inline int EncodeMorton3(const XMINT3 &v) {
	return (LeftShift3(v.z) << 2) | (LeftShift3(v.y) << 1) | LeftShift3(v.x);
}

HBVHTree::HBVHTree(const shared_ptr<NXScene>& scene)
{
	m_scene = scene;
}

void HBVHTree::BuildTreesWithScene(HBVHSplitMode mode)
{
	printf("Building BVH tree for scene...\n");

	if (!m_scene)
	{
		printf("ERROR: Can not create BVH trees. scene has pointed to nullptr.\n");
		return;
	}

	auto time_st = GetTickCount64();

	m_buildMode = mode;

	root = new HBVHTreeNode();
	int count = 0;	// �����е�primitive����
	int skipCount = 0;	// ������primitive����������line֮�಻�����ཻ�����primitive���ᱻ��������
	auto pPrimitives = m_scene->GetPrimitives();
	if (mode != HLBVH)
	{
		for (auto it = pPrimitives.begin(); it < pPrimitives.end(); it++)
		{
			//if ((*it)->GetRenderType() == eRenderType::Shape)
			{
				HBVHPrimitiveInfo primitiveInfo;
				primitiveInfo.index = count++;
				primitiveInfo.aabb = (*it)->GetAABBWorld();
				m_primitiveInfo.push_back(primitiveInfo);
			}
			//else
			//{
			//	count++;
			//	skipCount++;
			//}
		}

		BuildTree(root, 0, count - skipCount, mode);
	}
	else
	{
		for (auto it = pPrimitives.begin(); it < pPrimitives.end(); it++)
		{
			//if ((*it)->GetRenderType() == eRenderType::Shape)
			{
				HBVHMortonPrimitiveInfo primitiveInfo;
				primitiveInfo.index = count++;
				primitiveInfo.aabb = (*it)->GetAABBWorld();
				Vector3 fRelativePosition = m_scene->GetAABB().Offset(primitiveInfo.aabb.Center);
				int mortonScale = 1 << 10;
				XMINT3 iRelativePositionScaled = { (int)(fRelativePosition.x * mortonScale), (int)(fRelativePosition.y * mortonScale), (int)(fRelativePosition.z * mortonScale) };
				primitiveInfo.mortonCode = EncodeMorton3(iRelativePositionScaled);
				m_mortonPrimitiveInfo.push_back(primitiveInfo);
			}
		}

		sort(m_mortonPrimitiveInfo.begin(), m_mortonPrimitiveInfo.end(), [](const HBVHMortonPrimitiveInfo& a, const HBVHMortonPrimitiveInfo& b) {
			return a.mortonCode < b.mortonCode;
		});

		int mask = 0x3FFC0000;
		int start = 0;
		int lastMaskPos = m_mortonPrimitiveInfo[0].mortonCode & mask;
		int nPrimitiveLimit = 1000;
		for (int i = 1; i < count; i++)
		{
			int currentMaskPos = m_mortonPrimitiveInfo[i].mortonCode & mask;
			int nPrimitive = i - start;
			if (currentMaskPos != lastMaskPos || nPrimitive > nPrimitiveLimit)
			{
				HBVHTreeletInfo treelet;
				treelet.startIndex = start;
				treelet.nPrimitive = nPrimitive;
				m_treeletInfo.push_back(treelet);

				lastMaskPos = currentMaskPos;
				start = i;
			}
		}
		HBVHTreeletInfo treelet;
		treelet.startIndex = start;
		treelet.nPrimitive = count - start;
		m_treeletInfo.push_back(treelet);

		// ���̹߳�������treelet
		for (int i = 0; i < m_treeletInfo.size(); i++)
		{
			m_treeletInfo[i].node = BuildTreelet(m_treeletInfo[i].startIndex, m_treeletInfo[i].startIndex + m_treeletInfo[i].nPrimitive, 29 - 12);
		}

		// ����treelet������Ϻ󹹽��ϲ�������
		BuildUpperTree(root, 0, (int)m_treeletInfo.size());
	}

	auto time_ed = GetTickCount64();
	printf("BVH done. ��ʱ��%.3f ��\n", (float)(time_ed - time_st) / 1000.0f);
}

void HBVHTree::Intersect(const Ray& worldRay, NXHit& outHitInfo, float tMax)
{
	RecursiveIntersect(root, worldRay, outHitInfo, tMax);
}

void HBVHTree::BuildTree(HBVHTreeNode * node, int stIndex, int edIndex, HBVHSplitMode mode)
{
	//	�ݹ齨��
	//	���node��ֻ��һ���ڵ�͹���������
	//	�����ǰnode�£��������нڵ�Ŀ������ڽ�һ���ָ�Ŀ�����Ҳ����������
	//	����ָ��Ժ�ֲ���Ҳ����������
	//	��������¹����м�����

	for (int i = stIndex; i < edIndex; i++)
	{
		AABB::CreateMerged(node->aabb, node->aabb, m_primitiveInfo[i].aabb);
	}
	node->index = stIndex;
	node->offset = edIndex - stIndex;

	if (edIndex - stIndex == 1)
	{
		// isLeaf
		node->child[0] = nullptr;
		node->child[1] = nullptr;
		return;
	}

	if (edIndex - stIndex < SPLIT_COST)
	{
		// isLeaf
		node->child[0] = nullptr;
		node->child[1] = nullptr;
		return;
	}

	vector<HBVHPrimitiveInfo>::iterator itSplit;

	AABB centroidAABB;
	vector<Vector3> centroidAABBPoints;
	for (int i = stIndex; i < edIndex; i++)
	{
		centroidAABBPoints.push_back(m_primitiveInfo[i].aabb.Center);
	}
	AABB::CreateFromPoints(centroidAABB, centroidAABBPoints.size(), centroidAABBPoints.data(), sizeof(Vector3));

	int dim = centroidAABB.GetMaximumExtent();
	float startPos = centroidAABB.GetMax()[dim];
	float endPos = centroidAABB.GetMin()[dim];
	if (startPos == endPos)
	{
		// is leaf
		node->child[0] = nullptr;
		node->child[1] = nullptr;
		return;
	}

	switch (mode)
	{
	case SplitPosition:
	{
		float midPos = centroidAABB.GetCenter()[dim];
		itSplit = partition(m_primitiveInfo.begin() + stIndex, m_primitiveInfo.begin() + edIndex, [dim, midPos](HBVHPrimitiveInfo& info)
		{
			float boundPos = info.aabb.GetCenter()[dim];
			return boundPos < midPos;
		});
		break;
	}
	case SplitCount:
	{
		int midIndex = (stIndex + edIndex) / 2;
		itSplit = m_primitiveInfo.begin() + midIndex;
		nth_element(m_primitiveInfo.begin() + stIndex, itSplit, m_primitiveInfo.begin() + edIndex, [dim](HBVHPrimitiveInfo& a, HBVHPrimitiveInfo& b) {
			float aPos = a.aabb.GetCenter()[dim];
			float bPos = b.aabb.GetCenter()[dim];
			return aPos < bPos;
		});
		break;
	}
	case SAH:
	{
		// SAH ������
		// ����ǰ�ڵ���dim����ֳ�12�ݡ�Ȼ����м��11�ַָʽ��ȡһ��������ġ�
		// ����ͨ�����ӽڵ����������ķ�������Ȩ�صķ�ʽ�����ж��ĸ������㡣

		const int nBucket = 12;
		HBVHBucketInfo bucket[nBucket];
		for (auto it = m_primitiveInfo.begin() + stIndex; it != m_primitiveInfo.begin() + edIndex; it++)
		{
			int bucketPos = (int)(nBucket * centroidAABB.Offset(it->aabb.GetCenter())[dim]);
			bucketPos = Clamp(bucketPos, 0, nBucket - 1);
			AABB::CreateMerged(bucket[bucketPos].aabb, bucket[bucketPos].aabb, it->aabb);
			bucket[bucketPos].nPrimitive++;
		}

		float s = node->aabb.GetSurfaceArea();
		float cost[nBucket - 1];
		for (int i = 0; i < nBucket - 1; i++)
		{
			AABB abLeft, abRight;
			int nA = 0;
			int nB = 0;
			for (int j = 0; j < i + 1; j++)
			{
				AABB::CreateMerged(abLeft, abLeft, bucket[j].aabb);
				nA += bucket[j].nPrimitive;
			}

			for (int j = i + 1; j < nBucket; j++)
			{
				AABB::CreateMerged(abRight, abRight, bucket[j].aabb);
				nB += bucket[j].nPrimitive;
			}

			float sA = abLeft.GetSurfaceArea();
			float sB = abRight.GetSurfaceArea();
			float costValue = 1.0f + (sA * nA + sB * nB) / s;
			cost[i] = isnan(costValue) ? FLT_MAX : costValue;
		}

		// ��11���ָ����ȡ������ġ�
		float minCost = cost[0];
		int minCostBucket = 0;
		for (int i = 1; i < nBucket - 1; i++)
		{
			if (minCost > cost[i])
			{
				minCost = cost[i];
				minCostBucket = i;
			}
		}

		// ���������ķ����Ļ���ֵ��Ȼ�ȵ�ǰnode��ͼԪ�������࣬�ǻ�����ֱ�ӱ�������ͼԪ��
		if (minCost < node->offset * 10)
		{
			itSplit = partition(m_primitiveInfo.begin() + stIndex, m_primitiveInfo.begin() + edIndex, [=](HBVHPrimitiveInfo& info)
			{
				int bucketPos = (int)(nBucket * centroidAABB.Offset(info.aabb.GetCenter())[dim]);
				return bucketPos <= minCostBucket;
			});
		}
		else
		{
			// isLeaf
			node->child[0] = nullptr;
			node->child[1] = nullptr;
			return;
		}

		break;
	}
	default:
		break;
	}

	int splitPos = (int)(itSplit - m_primitiveInfo.begin());
	if (splitPos == stIndex || splitPos == edIndex)
	{
		// isLeaf
		node->child[0] = nullptr;
		node->child[1] = nullptr;
		return;
	}

	// isInterior
	node->child[0] = new HBVHTreeNode();
	node->child[1] = new HBVHTreeNode();
	BuildTree(node->child[0], stIndex, splitPos, mode);
	BuildTree(node->child[1], splitPos, edIndex, mode);
}

void HBVHTree::RecursiveIntersect(HBVHTreeNode* node, const Ray& worldRay, NXHit& outHitInfo, float& out_tHit)
{
	auto pPrimitives = m_scene->GetPrimitives();
	float t0, t1;
	auto v1 = node->aabb.GetMax();
	auto v2 = node->aabb.GetMin();
	if (worldRay.IntersectsFast(node->aabb, t0, t1))
	{
		float tNodeHit = t0;
		if (tNodeHit < 1e-5f) tNodeHit = t1;
		if (tNodeHit < 1e-5f)
			return;

		if (node->child[0] == nullptr && node->child[1] == nullptr)
		{
			if (m_buildMode == HLBVH)
			{
				// leaf
				for (int i = node->index; i < node->index + node->offset; i++)
				{
					for (int j = 0; j < m_treeletInfo[i].nPrimitive; j++)
					{
						int idx = m_treeletInfo[i].startIndex + j;

						//if (pPrimitives[m_mortonPrimitiveInfo[idx].index]->GetRenderType() == eRenderType::Shape)
						{
							//auto str = pPrimitives[m_mortonPrimitiveInfo[idx].index]->GetName();

							if (worldRay.IntersectsFast(m_mortonPrimitiveInfo[idx].aabb, t0, t1))
							{
								auto pPrim = pPrimitives[m_mortonPrimitiveInfo[idx].index];

								Matrix mxWorldInv = pPrim->GetWorldMatrixInv();
								Ray LocalRay = worldRay.Transform(mxWorldInv);

								float tHit;
								if (pPrim->RayCast(LocalRay, outHitInfo, tHit))
								{
									if (out_tHit > tHit)
									{
										out_tHit = tHit;
									}
								}
							}
						}
					}
				}
			}
			else
			{
				// leaf
				for (int i = node->index; i < node->index + node->offset; i++)
				{
					auto str = pPrimitives[m_primitiveInfo[i].index]->GetName();

					//if (pPrimitives[m_primitiveInfo[i].index]->GetRenderType() == eRenderType::Shape)
					{
						if (worldRay.IntersectsFast(m_primitiveInfo[i].aabb, t0, t1))
						{
							auto pPrim = pPrimitives[m_primitiveInfo[i].index];

							Matrix mxWorldInv = pPrim->GetWorldMatrixInv();
							Ray LocalRay = worldRay.Transform(mxWorldInv);

							float tHit;
							if (pPrim->RayCast(LocalRay, outHitInfo, tHit))
							{
								if (out_tHit > tHit)
								{
									out_tHit = tHit;
								}
							}
						}
					}
				}
			}
		}
		else
		{
			// interior
			RecursiveIntersect(node->child[0], worldRay, outHitInfo, out_tHit);
			RecursiveIntersect(node->child[1], worldRay, outHitInfo, out_tHit);
		}
	}
}

HBVHTreeNode* HBVHTree::BuildTreelet(int stIndex, int edIndex, int bitIndex)
{
	// ����Ѿ�û�еݹ�λ�ָֱ�Ӵ���һ������stIndex��edIndex����primitive���ӽڵ㣬��ֹͣ�ָ�
	// һ��ֵ���ôϸ��ʮ�ڷ�֮һ�������ϲ���ʣ��ʲô�ظ��Ķ����ˣ������ܵ�Ӱ�첻��
	if (bitIndex == -1)
	{
		HBVHTreeNode* result = new HBVHTreeNode();
		for (int i = stIndex; i < edIndex; i++)
		{
			AABB::CreateMerged(result->aabb, result->aabb, m_scene->GetPrimitives()[m_mortonPrimitiveInfo[i].index]->GetAABBWorld());
		}
		result->index = stIndex;
		result->offset = edIndex - stIndex;
		result->child[0] = result->child[1] = nullptr;

		return result;
	}

	// ���򣬼����м�λ
	int startMorton = m_mortonPrimitiveInfo[stIndex].mortonCode & bitIndex;
	int splitIndex = stIndex;
	for (int i = stIndex; i < edIndex; i++)
	{
		if ((m_mortonPrimitiveInfo[i].mortonCode & bitIndex) != startMorton)
		{
			splitIndex = i;
		}
	}

	// ����м�λû�ָܷ�����������ͽ�����һ���ָ�
	if (splitIndex == stIndex || splitIndex == edIndex)
	{
		return BuildTreelet(stIndex, edIndex, bitIndex - 1);
	}

	// �ָܷ���������ʹ�������ǰ�ڵ㣬���ݹ鴴����������
	HBVHTreeNode* result = new HBVHTreeNode();
	for (int i = stIndex; i < edIndex; i++)
	{
		AABB::CreateMerged(result->aabb, result->aabb, m_scene->GetPrimitives()[m_mortonPrimitiveInfo[i].index]->GetAABBWorld());
	}
	result->index = stIndex;
	result->offset = edIndex - stIndex;
	result->child[0] = BuildTreelet(stIndex, splitIndex, bitIndex - 1);
	result->child[1] = BuildTreelet(splitIndex, edIndex, bitIndex - 1);

	return result;
}

void HBVHTree::BuildUpperTree(HBVHTreeNode* node, int stIndex, int edIndex)
{
	for (int i = stIndex; i < edIndex; i++)
	{
		AABB::CreateMerged(node->aabb, node->aabb, m_treeletInfo[i].node->aabb);
	}

	node->index = stIndex;
	node->offset = edIndex - stIndex;
	if (edIndex - stIndex == 1)
	{
		node = m_treeletInfo[stIndex].node;
		return;
	}

	AABB centroidAABB;
	vector<Vector3> centroidAABBPoints;
	for (int i = stIndex; i < edIndex; i++)
	{
		centroidAABBPoints.push_back(m_treeletInfo[i].node->aabb.Center);
	}
	AABB::CreateFromPoints(centroidAABB, centroidAABBPoints.size(), centroidAABBPoints.data(), sizeof(Vector3));

	vector<HBVHTreeletInfo>::iterator itSplit;
	int dim = centroidAABB.GetMaximumExtent();
	const int nBucket = 12;
	HBVHBucketInfo bucket[nBucket];
	for (auto it = m_treeletInfo.begin() + stIndex; it != m_treeletInfo.begin() + edIndex; it++)
	{
		int bucketPos = (int)(nBucket * centroidAABB.Offset(it->node->aabb.GetCenter())[dim]);
		bucketPos = Clamp(bucketPos, 0, nBucket - 1);
		AABB::CreateMerged(bucket[bucketPos].aabb, bucket[bucketPos].aabb, it->node->aabb);
		bucket[bucketPos].nPrimitive += it->nPrimitive;
	}

	float s = node->aabb.GetSurfaceArea();
	float cost[nBucket - 1];
	for (int i = 0; i < nBucket - 1; i++)
	{
		AABB abLeft, abRight;
		int nA = 0;
		int nB = 0;
		for (int j = 0; j < i + 1; j++)
		{
			AABB::CreateMerged(abLeft, abLeft, bucket[j].aabb);
			nA += bucket[j].nPrimitive;
		}

		for (int j = i + 1; j < nBucket; j++)
		{
			AABB::CreateMerged(abRight, abRight, bucket[j].aabb);
			nB += bucket[j].nPrimitive;
		}

		float sA = abLeft.GetSurfaceArea();
		float sB = abRight.GetSurfaceArea();
		float costValue = 1.0f + (sA * nA + sB * nB) / s;
		cost[i] = isnan(costValue) ? FLT_MAX : costValue;
	}

	// ��11���ָ����ȡ������ġ�
	float minCost = cost[0];
	int minCostBucket = 0;
	for (int i = 1; i < nBucket - 1; i++)
	{
		if (minCost > cost[i])
		{
			minCost = cost[i];
			minCostBucket = i;
		}
	}

	// ����ͨSAH��ͬ��HLBVH���ϲ㽨����С��λ��һ����λ������
	// ��˲������ӽڵ�������treelet�Ѿ��������ӽڵ㣩��ֱ�ӹ����м�ڵ㼴�ɡ�
	itSplit = partition(m_treeletInfo.begin() + stIndex, m_treeletInfo.begin() + edIndex, [=](HBVHTreeletInfo& info)
	{
		int bucketPos = (int)(nBucket * centroidAABB.Offset(info.node->aabb.GetCenter())[dim]);
		return bucketPos <= minCostBucket;
	});

	int splitIndex = (int)(itSplit - m_treeletInfo.begin());
	node->child[0] = new HBVHTreeNode();
	node->child[1] = new HBVHTreeNode();
	BuildUpperTree(node->child[0], stIndex, splitIndex);
	BuildUpperTree(node->child[1], splitIndex, edIndex);
}

void HBVHTree::Release()
{
	m_treeletInfo.clear();
	m_mortonPrimitiveInfo.clear();
	m_primitiveInfo.clear();

	if (root)
	{
		ReleaseTreeNode(root);
	}
}

void HBVHTree::ReleaseTreeNode(HBVHTreeNode* node)
{
	if (node->child[0]) ReleaseTreeNode(node->child[0]); 
	if (node->child[1]) ReleaseTreeNode(node->child[1]);

	delete node;
}
