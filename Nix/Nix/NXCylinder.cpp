#include "NXCylinder.h"

NXCylinder::NXCylinder() :
	m_radius(0.0f),
	m_length(0.0f),
	m_segmentCircle(0.0f),
	m_segmentLength(0.0f)
{
}

void NXCylinder::Init(float radius, float length, int segmentCircle, int segmentLength)
{
	int currVertIdx = 0;

	float yBottom = -length * 0.5f;
	float yTop = length * 0.5f;

	Vector3 nBottom(0.0f, -1.0f, 0.0f);
	Vector3 nTop(0.0f, 1.0f, 0.0f);

	Vector3 pBottom(0.0f, yBottom, 0.0f);
	Vector3 pTop(0.0f, yTop, 0.0f);
	Vector2 uvBottomTop(0.5f, 0.5f);

	Vector3 tSide = Vector3(1.0f, 0.0f, 0.0f);	// 上下两个盖子的切线方向
	m_vertices.push_back({ pBottom, nBottom, uvBottomTop, tSide });
	currVertIdx++;
	float segmentCircleInv = 1.0f / (float)segmentCircle;
	for (int i = 0; i < segmentCircle; i++)
	{
		float segNow = (float)i * segmentCircleInv;
		float segNext = (float)(i + 1) * segmentCircleInv;
		float angleNow = segNow * XM_2PI;
		float angleNext = segNext * XM_2PI;

		float xNow = sinf(angleNow);
		float zNow = cosf(angleNow);
		float xNext = sinf(angleNext);
		float zNext = cosf(angleNext);

		Vector3 pNow(xNow * radius, yBottom, zNow * radius);
		Vector3 pNext(xNext * radius, yBottom, zNext * radius);

		Vector2 uvNow((xNow + 1.0f) * 0.5f, (zNow + 1.0f) * 0.5f);
		Vector2 uvNext((xNext + 1.0f) * 0.5f, (zNext + 1.0f) * 0.5f);
		m_vertices.push_back({ pNow, nBottom, uvNow, tSide });
		m_vertices.push_back({ pNext, nBottom, uvNext, tSide });
		
		m_indices.push_back(0);
		m_indices.push_back(currVertIdx + 1);
		m_indices.push_back(currVertIdx);
		
		currVertIdx += 2;
	}

	float segmentLengthInv = 1.0f / (float)segmentLength;
	for (int i = 0; i < segmentLength; i++)
	{
		float uvDown = (float)i * segmentLengthInv;
		float uvUp = (float)(i + 1) * segmentLengthInv;
		float yDown = uvDown * length + yBottom;
		float yUp = uvUp * length + yBottom;

		for (int j = 0; j < segmentCircle; j++)
		{
			float segNow = (float)j * segmentCircleInv;
			float segNext = (float)(j + 1) * segmentCircleInv;
			float angleNow = segNow * XM_2PI;
			float angleNext = segNext * XM_2PI;

			float xNow = sinf(angleNow);
			float zNow = cosf(angleNow);
			float xNext = sinf(angleNext);
			float zNext = cosf(angleNext);

			Vector3 pNowUp = { xNow * radius, yUp, zNow * radius };
			Vector3 pNextUp = { xNext * radius, yUp, zNext * radius };
			Vector3 pNowDown = { xNow * radius, yDown, zNow * radius };
			Vector3 pNextDown = { xNext * radius, yDown, zNext * radius };

			Vector3 nNow = { xNow, 0.0f, zNow };
			Vector3 nNext = { xNext, 0.0f, zNext };

			Vector2 uvNowUp		= { 1.0f - segNow,	1.0f - uvUp };
			Vector2 uvNextUp	= { 1.0f - segNext,	1.0f - uvUp };
			Vector2 uvNowDown	= { 1.0f - segNow,	1.0f - uvDown };
			Vector2 uvNextDown	= { 1.0f - segNext,	1.0f - uvDown };

			// 计算切线
			Vector3 vNowUp = { xNow, yUp, zNow };
			Vector3 vNextUp = { xNext, yUp, zNext };
			Vector3 vNowDown = { xNow, yDown, zNow };
			Vector3 vNextDown = { xNext, yDown, zNext };
			Vector3 tNowUp = vNowUp.Cross(nTop);
			Vector3 tNextUp = vNextUp.Cross(nTop);
			Vector3 tNowDown = vNowDown.Cross(nTop);
			Vector3 tNextDown = vNextDown.Cross(nTop);
			m_vertices.push_back({ pNowUp,		nNow,	uvNowUp,	 tNowUp    });
			m_vertices.push_back({ pNextUp,		nNext,	uvNextUp,	 tNextUp   });
			m_vertices.push_back({ pNowDown,	nNow,	uvNowDown,	 tNowDown  });
			m_vertices.push_back({ pNextDown,	nNext,	uvNextDown,	 tNextDown });

			m_indices.push_back(currVertIdx);
			m_indices.push_back(currVertIdx + 2);
			m_indices.push_back(currVertIdx + 1);
			m_indices.push_back(currVertIdx + 1);
			m_indices.push_back(currVertIdx + 2);
			m_indices.push_back(currVertIdx + 3);

			currVertIdx += 4;
		}
	}

	// 圆柱顶部
	m_vertices.push_back({ pTop, nTop, uvBottomTop, tSide });
	int SaveIdx = currVertIdx++;
	for (int i = 0; i < segmentCircle; i++)
	{
		float segNow = (float)i * segmentCircleInv;
		float segNext = (float)(i + 1) * segmentCircleInv;
		float angleNow = segNow * XM_2PI;
		float angleNext = segNext * XM_2PI;

		float xNow = sinf(angleNow);
		float zNow = cosf(angleNow);
		float xNext = sinf(angleNext);
		float zNext = cosf(angleNext);

		Vector3 pNow(xNow * radius, yTop, zNow * radius);
		Vector3 pNext(xNext * radius, yTop, zNext * radius);

		Vector2 uvNow((xNow + 1.0f) * 0.5f, (1.0f - zNow) * 0.5f);
		Vector2 uvNext((xNext + 1.0f) * 0.5f, (1.0f - zNext) * 0.5f);
		m_vertices.push_back({ pNow, nTop, uvNow, tSide });
		m_vertices.push_back({ pNext, nTop, uvNext, tSide });
		
		m_indices.push_back(SaveIdx);
		m_indices.push_back(currVertIdx);
		m_indices.push_back(currVertIdx + 1);
		
		currVertIdx += 2;
	}

	InitVertexIndexBuffer();
	InitAABB();
}


