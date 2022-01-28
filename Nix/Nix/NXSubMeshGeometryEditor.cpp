#include "NXSubMeshGeometryEditor.h"
#include "NXPrimitive.h"
#include "FBXMeshLoader.h"
#include "SphereHarmonics.h"

NXSubMeshGeometryEditor::NXSubMeshGeometryEditor()
{
}

NXSubMeshGeometryEditor::~NXSubMeshGeometryEditor()
{
}

void NXSubMeshGeometryEditor::CreateFBXMesh(NXPrimitive* pMesh, UINT subMeshCount, VertexPNTT** pSubMeshVertices, UINT* pSubMeshVerticesCounts, UINT** pSubMeshIndices, UINT* pSubMeshIndicesCounts, bool bAutoCalcTangents)
{
	pMesh->ClearSubMeshes();

	pMesh->ResizeSubMesh(subMeshCount);
	for (UINT i = 0; i < subMeshCount; i++)
	{
		NXSubMesh* pSubMesh = new NXSubMesh(pMesh);

		pSubMesh->m_vertices.reserve(pSubMeshVerticesCounts[i]);
		pSubMesh->m_vertices.assign(pSubMeshVertices[i], pSubMeshVertices[i] + pSubMeshVerticesCounts[i]);

		pSubMesh->m_indices.reserve(pSubMeshIndicesCounts[i]);
		pSubMesh->m_indices.assign(pSubMeshIndices[i], pSubMeshIndices[i] + pSubMeshIndicesCounts[i]);

		if (bAutoCalcTangents) pSubMesh->CalculateTangents();

		pSubMesh->InitVertexIndexBuffer();
		pMesh->ReloadSubMesh(i, pSubMesh);
	}

	pMesh->InitAABB();
}

void NXSubMeshGeometryEditor::CreateBox(NXPrimitive* pMesh, float x, float y, float z)
{
	x *= 0.5f;
	y *= 0.5f;
	z *= 0.5f;

	pMesh->ClearSubMeshes();
	NXSubMesh* pSubMesh = new NXSubMesh(pMesh);
	pSubMesh->m_vertices =
	{
		// -X
		{ Vector3(-x, -y, +z), Vector3(-1.0f, 0.0f, 0.0f),	Vector2(0.0f, 1.0f), Vector3(0.0f, 0.0f, -1.0f) },
		{ Vector3(-x, +y, +z), Vector3(-1.0f, 0.0f, 0.0f),	Vector2(0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f) },
		{ Vector3(-x, +y, -z), Vector3(-1.0f, 0.0f, 0.0f),	Vector2(1.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f) },
		{ Vector3(-x, -y, -z), Vector3(-1.0f, 0.0f, 0.0f),	Vector2(1.0f, 1.0f), Vector3(0.0f, 0.0f, -1.0f) },

		// +X
		{ Vector3(+x, -y, -z), Vector3(1.0f, 0.0f, 0.0f),	Vector2(0.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f)  },
		{ Vector3(+x, +y, -z), Vector3(1.0f, 0.0f, 0.0f),	Vector2(0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f)  },
		{ Vector3(+x, +y, +z), Vector3(1.0f, 0.0f, 0.0f),	Vector2(1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f)  },
		{ Vector3(+x, -y, +z), Vector3(1.0f, 0.0f, 0.0f),	Vector2(1.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f)  },

		// -Y
		{ Vector3(-x, -y, -z), Vector3(0.0f, -1.0f, 0.0f),	Vector2(0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f)  },
		{ Vector3(+x, -y, -z), Vector3(0.0f, -1.0f, 0.0f),	Vector2(1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f)  },
		{ Vector3(+x, -y, +z), Vector3(0.0f, -1.0f, 0.0f),	Vector2(1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f)  },
		{ Vector3(-x, -y, +z), Vector3(0.0f, -1.0f, 0.0f),	Vector2(0.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f)  },

		// +Y
		{ Vector3(-x, +y, +z), Vector3(0.0f, 1.0f, 0.0f),	Vector2(0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f)  },
		{ Vector3(+x, +y, +z), Vector3(0.0f, 1.0f, 0.0f),	Vector2(1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f)  },
		{ Vector3(+x, +y, -z), Vector3(0.0f, 1.0f, 0.0f),	Vector2(1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f)  },
		{ Vector3(-x, +y, -z), Vector3(0.0f, 1.0f, 0.0f),	Vector2(0.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f)  },

		// -Z
		{ Vector3(-x, +y, -z), Vector3(0.0f, 0.0f, -1.0f),	Vector2(0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f)  },
		{ Vector3(+x, +y, -z), Vector3(0.0f, 0.0f, -1.0f),	Vector2(1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f)  },
		{ Vector3(+x, -y, -z), Vector3(0.0f, 0.0f, -1.0f),	Vector2(1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f)  },
		{ Vector3(-x, -y, -z), Vector3(0.0f, 0.0f, -1.0f),	Vector2(0.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f)  },

		// +Z
		{ Vector3(-x, -y, +z), Vector3(0.0f, 0.0f, 1.0f),	Vector2(1.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f)  },
		{ Vector3(+x, -y, +z), Vector3(0.0f, 0.0f, 1.0f),	Vector2(0.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f)  },
		{ Vector3(+x, +y, +z), Vector3(0.0f, 0.0f, 1.0f),	Vector2(0.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f)  },
		{ Vector3(-x, +y, +z), Vector3(0.0f, 0.0f, 1.0f),	Vector2(1.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f)  },
	};

	pSubMesh->m_indices =
	{
		0,  1,  2,
		0,  2,  3,

		4,  5,  6,
		4,  6,  7,

		8,  9,  10,
		8,  10, 11,

		12, 13, 14,
		12, 14, 15,

		16, 17, 18,
		16, 18, 19,

		20, 21, 22,
		20, 22, 23
	};

	pSubMesh->InitVertexIndexBuffer();

	pMesh->AddSubMesh(pSubMesh);
	pMesh->InitAABB();
}

void NXSubMeshGeometryEditor::CreateCylinder(NXPrimitive* pMesh, float radius, float length, int segmentCircle, int segmentLength)
{
	pMesh->ClearSubMeshes();
	NXSubMesh* pSubMesh = new NXSubMesh(pMesh);

	int currVertIdx = 0;

	float yBottom = -length * 0.5f;
	float yTop = length * 0.5f;

	Vector3 nBottom(0.0f, -1.0f, 0.0f);
	Vector3 nTop(0.0f, 1.0f, 0.0f);

	Vector3 pBottom(0.0f, yBottom, 0.0f);
	Vector3 pTop(0.0f, yTop, 0.0f);
	Vector2 uvBottomTop(0.5f, 0.5f);

	Vector3 tSide = Vector3(1.0f, 0.0f, 0.0f);	// 上下两个盖子的切线方向
	pSubMesh->m_vertices.push_back({ pBottom, nBottom, uvBottomTop, tSide });
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
		pSubMesh->m_vertices.push_back({ pNow, nBottom, uvNow, tSide });
		pSubMesh->m_vertices.push_back({ pNext, nBottom, uvNext, tSide });

		pSubMesh->m_indices.push_back(0);
		pSubMesh->m_indices.push_back(currVertIdx + 1);
		pSubMesh->m_indices.push_back(currVertIdx);

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

			Vector2 uvNowUp = { 1.0f - segNow,	1.0f - uvUp };
			Vector2 uvNextUp = { 1.0f - segNext,	1.0f - uvUp };
			Vector2 uvNowDown = { 1.0f - segNow,	1.0f - uvDown };
			Vector2 uvNextDown = { 1.0f - segNext,	1.0f - uvDown };

			// 计算切线
			Vector3 vNow = { xNow, 0.0f, zNow };
			Vector3 vNext = { xNext, 0.0f, zNext };

			Vector3 tNowUp = vNow.Cross(nTop);
			Vector3 tNextUp = vNext.Cross(nTop);
			Vector3 tNowDown = vNow.Cross(nTop);
			Vector3 tNextDown = vNext.Cross(nTop);
			pSubMesh->m_vertices.push_back({ pNowUp,		nNow,	uvNowUp,	 tNowUp });
			pSubMesh->m_vertices.push_back({ pNextUp,		nNext,	uvNextUp,	 tNextUp });
			pSubMesh->m_vertices.push_back({ pNowDown,	nNow,	uvNowDown,	 tNowDown });
			pSubMesh->m_vertices.push_back({ pNextDown,	nNext,	uvNextDown,	 tNextDown });

			pSubMesh->m_indices.push_back(currVertIdx);
			pSubMesh->m_indices.push_back(currVertIdx + 2);
			pSubMesh->m_indices.push_back(currVertIdx + 1);
			pSubMesh->m_indices.push_back(currVertIdx + 1);
			pSubMesh->m_indices.push_back(currVertIdx + 2);
			pSubMesh->m_indices.push_back(currVertIdx + 3);

			currVertIdx += 4;
		}
	}

	// 圆柱顶部
	pSubMesh->m_vertices.push_back({ pTop, nTop, uvBottomTop, tSide });
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
		pSubMesh->m_vertices.push_back({ pNow, nTop, uvNow, tSide });
		pSubMesh->m_vertices.push_back({ pNext, nTop, uvNext, tSide });

		pSubMesh->m_indices.push_back(SaveIdx);
		pSubMesh->m_indices.push_back(currVertIdx);
		pSubMesh->m_indices.push_back(currVertIdx + 1);

		currVertIdx += 2;
	}

	pSubMesh->InitVertexIndexBuffer();

	pMesh->AddSubMesh(pSubMesh);
	pMesh->InitAABB();
}

void NXSubMeshGeometryEditor::CreatePlane(NXPrimitive* pMesh, float width, float height, NXPlaneAxis Axis)
{
	pMesh->ClearSubMeshes();
	NXSubMesh* pSubMesh = new NXSubMesh(pMesh);

	float w = width * 0.5f, h = height * 0.5f;
	switch (Axis)
	{
	case POSITIVE_X:
		pSubMesh->m_vertices =
		{
			{ Vector3(+0.0f, -w, -h), Vector3(1.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f) },
			{ Vector3(+0.0f, +w, -h), Vector3(1.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) },
			{ Vector3(+0.0f, +w, +h), Vector3(1.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) },
			{ Vector3(+0.0f, -w, +h), Vector3(1.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f) },
		};
		break;
	case POSITIVE_Y:
		pSubMesh->m_vertices =
		{
			{ Vector3(-w, +0.0f, +h), Vector3(0.0f, 1.0f, 0.0f), Vector2(0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f) },
			{ Vector3(+w, +0.0f, +h), Vector3(0.0f, 1.0f, 0.0f), Vector2(1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f) },
			{ Vector3(+w, +0.0f, -h), Vector3(0.0f, 1.0f, 0.0f), Vector2(1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f) },
			{ Vector3(-w, +0.0f, -h), Vector3(0.0f, 1.0f, 0.0f), Vector2(0.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f) },
		};
		break;
	case POSITIVE_Z:
		pSubMesh->m_vertices =
		{
			{ Vector3(-w, -h, +0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(1.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f) },
			{ Vector3(+w, -h, +0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f) },
			{ Vector3(+w, +h, +0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(0.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f) },
			{ Vector3(-w, +h, +0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector2(1.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f) },
		};
		break;
	case NEGATIVE_X:
		pSubMesh->m_vertices =
		{
			{ Vector3(0.0f, -w, +h), Vector3(-1.0f, 0.0f, 0.0f), Vector2(0.0f, 1.0f), Vector3(0.0f, 0.0f, -1.0f) },
			{ Vector3(0.0f, +w, +h), Vector3(-1.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f) },
			{ Vector3(0.0f, +w, -h), Vector3(-1.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f) },
			{ Vector3(0.0f, -w, -h), Vector3(-1.0f, 0.0f, 0.0f), Vector2(1.0f, 1.0f), Vector3(0.0f, 0.0f, -1.0f) },
		};
		break;
	case NEGATIVE_Y:
		pSubMesh->m_vertices =
		{
			{ Vector3(-w, -0.0f, -h), Vector3(0.0f, -1.0f, 0.0f), Vector2(0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f) },
			{ Vector3(+w, -0.0f, -h), Vector3(0.0f, -1.0f, 0.0f), Vector2(1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f) },
			{ Vector3(+w, -0.0f, +h), Vector3(0.0f, -1.0f, 0.0f), Vector2(1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f) },
			{ Vector3(-w, -0.0f, +h), Vector3(0.0f, -1.0f, 0.0f), Vector2(0.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f) },
		};
		break;
	case NEGATIVE_Z:
		pSubMesh->m_vertices =
		{
			{ Vector3(-w, +h, -0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector2(0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f) },
			{ Vector3(+w, +h, -0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector2(1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f) },
			{ Vector3(+w, -h, -0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector2(1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f) },
			{ Vector3(-w, -h, -0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector2(0.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f) },
		};
		break;
	}

	pSubMesh->m_indices =
	{
		0,  1,  2,
		0,  2,  3
	};

	pSubMesh->InitVertexIndexBuffer();

	pMesh->AddSubMesh(pSubMesh);
	pMesh->InitAABB();
}

void NXSubMeshGeometryEditor::CreateSphere(NXPrimitive* pMesh, float radius, int segmentHorizontal, int segmentVertical)
{
	pMesh->ClearSubMeshes();
	NXSubMesh* pSubMesh = new NXSubMesh(pMesh);

	Vector3 vTop(0.0f, 1.0f, 0.0f);
	Vector3 vBottom(0.0f, -1.0f, 0.0f);

	int currVertIdx = 0;
	for (int i = 0; i < segmentVertical; i++)
	{
		float yDown = sinf(((float)i / (float)segmentVertical * 2.0f - 1.0f) * XM_PIDIV2) * radius;
		float yUp = sinf(((float)(i + 1) / (float)segmentVertical * 2.0f - 1.0f) * XM_PIDIV2) * radius;
		float radiusDown = sqrtf(radius * radius - yDown * yDown);
		float radiusUp = sqrtf(radius * radius - yUp * yUp);

		float yUVUp = 1.0f - Clamp(yUp * 0.5f + 0.5f, 0.0f, 1.0f);
		float yUVDown = 1.0f - Clamp(yDown * 0.5f + 0.5f, 0.0f, 1.0f);

		for (int j = 0; j < segmentHorizontal; j++)
		{
			float segNow = (float)j / (float)segmentHorizontal;
			float segNext = (float)(j + 1) / (float)segmentHorizontal;
			float angleNow = segNow * XM_2PI;
			float angleNext = segNext * XM_2PI;
			float xNow = cosf(angleNow);
			float zNow = sinf(angleNow);
			float xNext = cosf(angleNext);
			float zNext = sinf(angleNext);

			Vector3 pNowUp = { xNow * radiusUp, yUp, zNow * radiusUp };
			Vector3 pNextUp = { xNext * radiusUp, yUp, zNext * radiusUp };
			Vector3 pNowDown = { xNow * radiusDown, yDown, zNow * radiusDown };
			Vector3 pNextDown = { xNext * radiusDown, yDown, zNext * radiusDown };

			Vector2 uvNowUp = { segNow, yUVUp };
			Vector2 uvNextUp = { segNext, yUVUp };
			Vector2 uvNowDown = { segNow, yUVDown };
			Vector2 uvNextDown = { segNext, yUVDown };

			float invRadius = 1.0f / radius;
			Vector3 nNowUp, nNowDown, nNextUp, nNextDown;
			nNowUp = pNowUp * invRadius;
			nNextUp = pNextUp * invRadius;
			nNowDown = pNowDown * invRadius;
			nNextDown = pNextDown * invRadius;

			// 计算切线
			Vector3 vNow = { xNow, 0.0f, zNow };
			Vector3 vNext = { xNext, 0.0f, zNext };

			Vector3 tNowUp = vNow.Cross(vTop);
			Vector3 tNextUp = vNext.Cross(vTop);
			Vector3 tNowDown = vNow.Cross(vTop);
			Vector3 tNextDown = vNext.Cross(vTop);
			tNowUp.Normalize();
			tNextUp.Normalize();
			tNowDown.Normalize();
			tNextDown.Normalize();

			pSubMesh->m_vertices.push_back({ pNowUp,	nNowUp,		uvNowUp,	tNowUp });
			pSubMesh->m_vertices.push_back({ pNextUp,	nNextUp,	uvNextUp,	tNextUp });
			pSubMesh->m_vertices.push_back({ pNextDown,	nNextDown,	uvNextDown, tNextDown });
			pSubMesh->m_vertices.push_back({ pNowDown,	nNowDown,	uvNowDown,	tNowDown });

			pSubMesh->m_indices.push_back(currVertIdx);
			pSubMesh->m_indices.push_back(currVertIdx + 1);
			pSubMesh->m_indices.push_back(currVertIdx + 2);
			pSubMesh->m_indices.push_back(currVertIdx);
			pSubMesh->m_indices.push_back(currVertIdx + 2);
			pSubMesh->m_indices.push_back(currVertIdx + 3);

			currVertIdx += 4;
		}
	}

	pSubMesh->InitVertexIndexBuffer();

	pMesh->AddSubMesh(pSubMesh);
	pMesh->InitAABB();
}

void NXSubMeshGeometryEditor::CreateSHSphere(NXPrimitive* pMesh, int basis_l, int basis_m, float radius, int segmentHorizontal, int segmentVertical)
{
	pMesh->ClearSubMeshes();
	NXSubMesh* pSubMesh = new NXSubMesh(pMesh);

	Vector3 vTop(0.0f, 1.0f, 0.0f);
	Vector3 vBottom(0.0f, -1.0f, 0.0f);

	int currVertIdx = 0;
	for (int i = 0; i < segmentVertical; i++)
	{
		float segDown = (float)i / (float)segmentVertical;
		float segUp = (float)(i + 1) / (float)segmentVertical;
		float angleDown = segDown * XM_PI - XM_PIDIV2;
		float angleUp = segUp * XM_PI - XM_PIDIV2;

		float yDown = sinf(angleDown) * radius;
		float yUp = sinf(angleUp) * radius;
		float radiusDown = sqrtf(radius * radius - yDown * yDown);
		float radiusUp = sqrtf(radius * radius - yUp * yUp);

		float yUVUp = 1.0f - Clamp(yUp * 0.5f + 0.5f, 0.0f, 1.0f);
		float yUVDown = 1.0f - Clamp(yDown * 0.5f + 0.5f, 0.0f, 1.0f);

		for (int j = 0; j < segmentHorizontal; j++)
		{
			float segNow = (float)j / (float)segmentHorizontal;
			float segNext = (float)(j + 1) / (float)segmentHorizontal;
			float angleNow = segNow * XM_2PI;
			float angleNext = segNext * XM_2PI;
			float xNow = cosf(angleNow);
			float zNow = sinf(angleNow);
			float xNext = cosf(angleNext);
			float zNext = sinf(angleNext);

			float shNowUp =		DirectX::SimpleMath::SH::SHBasis(basis_l, basis_m, 2.0f * angleUp, 2.0f * angleNow);
			float shNextUp =	DirectX::SimpleMath::SH::SHBasis(basis_l, basis_m, 2.0f * angleUp, 2.0f * angleNext);
			float shNowDown =	DirectX::SimpleMath::SH::SHBasis(basis_l, basis_m, 2.0f * angleDown, 2.0f * angleNow);
			float shNextDown =	DirectX::SimpleMath::SH::SHBasis(basis_l, basis_m, 2.0f * angleDown, 2.0f * angleNext);

			Vector3 pNowUp = { xNow * radiusUp, yUp, zNow * radiusUp };
			Vector3 pNextUp = { xNext * radiusUp, yUp, zNext * radiusUp };
			Vector3 pNowDown = { xNow * radiusDown, yDown, zNow * radiusDown };
			Vector3 pNextDown = { xNext * radiusDown, yDown, zNext * radiusDown };
			pNowUp *= shNowUp;
			pNextUp *= shNextUp;
			pNowDown *= shNowDown;
			pNextDown *= shNextDown;

			Vector2 uvNowUp = { segNow, yUVUp };
			Vector2 uvNextUp = { segNext, yUVUp };
			Vector2 uvNowDown = { segNow, yUVDown };
			Vector2 uvNextDown = { segNext, yUVDown };

			Vector3 nNowUp = pNowUp;
			Vector3 nNextUp = pNextUp;
			Vector3 nNowDown = pNowDown;
			Vector3 nNextDown = pNextDown;
			nNowUp.Normalize();
			nNextUp.Normalize();
			nNowDown.Normalize();
			nNextDown.Normalize();

			// 计算切线
			Vector3 vNow = { xNow, 0.0f, zNow };
			Vector3 vNext = { xNext, 0.0f, zNext };

			Vector3 tNowUp = vNow.Cross(vTop);
			Vector3 tNextUp = vNext.Cross(vTop);
			Vector3 tNowDown = vNow.Cross(vTop);
			Vector3 tNextDown = vNext.Cross(vTop);
			tNowUp.Normalize();
			tNextUp.Normalize();
			tNowDown.Normalize();
			tNextDown.Normalize();

			pSubMesh->m_vertices.push_back({ pNowUp,	nNowUp,		uvNowUp,	tNowUp });
			pSubMesh->m_vertices.push_back({ pNextUp,	nNextUp,	uvNextUp,	tNextUp });
			pSubMesh->m_vertices.push_back({ pNextDown,	nNextDown,	uvNextDown, tNextDown });
			pSubMesh->m_vertices.push_back({ pNowDown,	nNowDown,	uvNowDown,	tNowDown });

			pSubMesh->m_indices.push_back(currVertIdx);
			pSubMesh->m_indices.push_back(currVertIdx + 1);
			pSubMesh->m_indices.push_back(currVertIdx + 2);
			pSubMesh->m_indices.push_back(currVertIdx);
			pSubMesh->m_indices.push_back(currVertIdx + 2);
			pSubMesh->m_indices.push_back(currVertIdx + 3);

			currVertIdx += 4;
		}
	}

	pSubMesh->InitVertexIndexBuffer();

	pMesh->AddSubMesh(pSubMesh);
	pMesh->InitAABB();
}
