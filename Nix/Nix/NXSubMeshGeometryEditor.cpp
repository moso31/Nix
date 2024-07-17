#include "NXSubMeshGeometryEditor.h"
#include "NXPrimitive.h"
#include "NXPrefab.h"
#include "FBXMeshLoader.h"
#include "SphereHarmonics.h"
#include "NXSubMesh.h" // include this .h for EditorObjectID only.

NXSubMeshGeometryEditor::NXSubMeshGeometryEditor()
{
}

NXSubMeshGeometryEditor::~NXSubMeshGeometryEditor()
{
}

void NXSubMeshGeometryEditor::Init(ID3D12Device* pDevice)
{
	m_pDevice = pDevice;
	m_vbAllocator = new CommittedAllocator(m_pDevice.Get());
	m_ibAllocator = new CommittedAllocator(m_pDevice.Get());

	NX12Util::CreateCommands(m_pDevice.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandQueue.GetAddressOf(), m_pCommandAllocator.GetAddressOf(), m_pCommandList.GetAddressOf());
	m_pCommandQueue->SetName(L"NXSubMeshGeometryEditor Command Queue");
	m_pFence = NX12Util::CreateFence(m_pDevice.Get());

	InitCommonMeshes();
}

void NXSubMeshGeometryEditor::CreateFBXPrefab(NXPrefab* pPrefab, const std::string& filePath, bool bAutoCalcTangents)
{
	FBXMeshLoader::LoadFBXFile(filePath, pPrefab, bAutoCalcTangents);
}

void NXSubMeshGeometryEditor::CreateBox(NXPrimitive* pMesh, float x, float y, float z)
{
	x *= 0.5f;
	y *= 0.5f;
	z *= 0.5f;

	pMesh->ClearSubMeshes();
	NXSubMeshStandard* pSubMesh = new NXSubMeshStandard(pMesh, "_Box");
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

	pSubMesh->TryAddBuffers();

	pMesh->AddSubMesh(pSubMesh);
}

void NXSubMeshGeometryEditor::CreateCylinder(NXPrimitive* pMesh, float radius, float length, int segmentCircle, int segmentLength)
{
	pMesh->ClearSubMeshes();
	NXSubMeshStandard* pSubMesh = new NXSubMeshStandard(pMesh, "_Cylinder");

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

	pSubMesh->TryAddBuffers();

	pMesh->AddSubMesh(pSubMesh);
}

void NXSubMeshGeometryEditor::CreatePlane(NXPrimitive* pMesh, float width, float height, NXPlaneAxis Axis)
{
	pMesh->ClearSubMeshes();
	NXSubMeshStandard* pSubMesh = new NXSubMeshStandard(pMesh, "_Plane");

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

	pSubMesh->TryAddBuffers();

	pMesh->AddSubMesh(pSubMesh);
}

void NXSubMeshGeometryEditor::CreateSphere(NXPrimitive* pMesh, float radius, int segmentHorizontal, int segmentVertical)
{
	pMesh->ClearSubMeshes();
	NXSubMeshStandard* pSubMesh = new NXSubMeshStandard(pMesh, "_Sphere");

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

	pSubMesh->TryAddBuffers();

	pMesh->AddSubMesh(pSubMesh);
}

void NXSubMeshGeometryEditor::CreateSHSphere(NXPrimitive* pMesh, int basis_l, int basis_m, float radius, int segmentHorizontal, int segmentVertical)
{
	pMesh->ClearSubMeshes();
	NXSubMeshStandard* pSubMesh = new NXSubMeshStandard(pMesh, "_SHSphere");

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

			float shNowUp =		DirectX::SimpleMath::SH::SHBasis(basis_l, basis_m, XM_PIDIV2 + angleUp, angleNow);
			float shNextUp =	DirectX::SimpleMath::SH::SHBasis(basis_l, basis_m, XM_PIDIV2 + angleUp, angleNext);
			float shNowDown =	DirectX::SimpleMath::SH::SHBasis(basis_l, basis_m, XM_PIDIV2 + angleDown, angleNow);
			float shNextDown =	DirectX::SimpleMath::SH::SHBasis(basis_l, basis_m, XM_PIDIV2 + angleDown, angleNext);

			Vector3 pNowUp = { xNow * radiusUp, yUp, zNow * radiusUp };
			Vector3 pNextUp = { xNext * radiusUp, yUp, zNext * radiusUp };
			Vector3 pNowDown = { xNow * radiusDown, yDown, zNow * radiusDown };
			Vector3 pNextDown = { xNext * radiusDown, yDown, zNext * radiusDown };
			pNowUp *= abs(shNowUp);
			pNextUp *= abs(shNextUp);
			pNowDown *= abs(shNowDown);
			pNextDown *= abs(shNextDown);

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

	pSubMesh->TryAddBuffers();

	pMesh->AddSubMesh(pSubMesh);
}

void NXSubMeshGeometryEditor::CreateMoveArrows(NXPrimitive* pMesh)
{
	Vector4 colorX(1.0f, 0.0f, 0.0f, 1.0f);
	Vector4 colorY(0.0f, 1.0f, 0.0f, 1.0f);
	Vector4 colorZ(0.0f, 0.0f, 1.0f, 1.0f);

	float fSegmentCircleInv = 1.0f / 16.0f;
	float fRadius = 0.025f;
	float fCylinderLo = -fRadius;
	float fCylinderHi = 1.0f;

	int meshId = 0;
	for (int i = 0; i < 3; i++)
	{
		UINT currVertIdx = 0;

		EditorObjectID objId = i == 0 ? EditorObjectID::TRANSLATE_X : i == 1 ? EditorObjectID::TRANSLATE_Y : EditorObjectID::TRANSLATE_Z;
		NXSubMeshEditorObjects* pSubMesh = new NXSubMeshEditorObjects(pMesh, "_MoveArrows_" + std::to_string(meshId++), objId);
		for (int segIdx = 0; segIdx < 16; segIdx++)
		{
			float angleCurr = (float)(segIdx + 0) * fSegmentCircleInv * XM_2PI;
			float angleNext = (float)(segIdx + 1) * fSegmentCircleInv * XM_2PI;

			float sinCurr = sinf(angleCurr);
			float cosCurr = cosf(angleCurr);
			float sinNext = sinf(angleNext);
			float cosNext = cosf(angleNext);

			Vector3 p00, p01, p10, p11;
			if (i == 0)
			{
				p00 = Vector3(fCylinderLo, sinCurr * fRadius, cosCurr * fRadius);
				p01 = Vector3(fCylinderLo, sinNext * fRadius, cosNext * fRadius);
				p10 = Vector3(fCylinderHi, sinCurr * fRadius, cosCurr * fRadius);
				p11 = Vector3(fCylinderHi, sinNext * fRadius, cosNext * fRadius);
			}
			else if (i == 1)
			{
				p00 = Vector3(sinCurr * fRadius, fCylinderLo, cosCurr * fRadius);
				p01 = Vector3(sinNext * fRadius, fCylinderLo, cosNext * fRadius);
				p10 = Vector3(sinCurr * fRadius, fCylinderHi, cosCurr * fRadius);
				p11 = Vector3(sinNext * fRadius, fCylinderHi, cosNext * fRadius);
			}
			else // i == 2
			{
				p00 = Vector3(sinCurr * fRadius, cosCurr * fRadius, fCylinderLo);
				p01 = Vector3(sinNext * fRadius, cosNext * fRadius, fCylinderLo);
				p10 = Vector3(sinCurr * fRadius, cosCurr * fRadius, fCylinderHi);
				p11 = Vector3(sinNext * fRadius, cosNext * fRadius, fCylinderHi);
			}

			Vector4 color = (i == 0 ? colorX : i == 1 ? colorY : colorZ);
			pSubMesh->m_vertices.push_back({ p00, color });
			pSubMesh->m_vertices.push_back({ p01, color });
			pSubMesh->m_vertices.push_back({ p10, color });
			pSubMesh->m_vertices.push_back({ p11, color });

			pSubMesh->m_indices.push_back(currVertIdx);
			pSubMesh->m_indices.push_back(currVertIdx + 2);
			pSubMesh->m_indices.push_back(currVertIdx + 1);
			pSubMesh->m_indices.push_back(currVertIdx + 1);
			pSubMesh->m_indices.push_back(currVertIdx + 2);
			pSubMesh->m_indices.push_back(currVertIdx + 3);

			currVertIdx += 4;
		}

		pSubMesh->TryAddBuffers();
		pMesh->AddSubMesh(pSubMesh);
	}

	float fConeRadius = 0.1f;
	float fConeLo = fCylinderHi;
	float fConeHi = fCylinderHi + 0.4f;
	for (int i = 0; i < 3; i++)
	{
		UINT currVertIdx = 0;
		EditorObjectID objId = i == 0 ? EditorObjectID::TRANSLATE_X : i == 1 ? EditorObjectID::TRANSLATE_Y : EditorObjectID::TRANSLATE_Z;
		NXSubMeshEditorObjects* pSubMesh = new NXSubMeshEditorObjects(pMesh, "_MoveArrows" + std::to_string(meshId++), objId);
		for (int segIdx = 0; segIdx < 16; segIdx++)
		{
			float angleCurr = (float)(segIdx + 0) * fSegmentCircleInv * XM_2PI;
			float angleNext = (float)(segIdx + 1) * fSegmentCircleInv * XM_2PI;

			float sinCurr = sinf(angleCurr);
			float cosCurr = cosf(angleCurr);
			float sinNext = sinf(angleNext);
			float cosNext = cosf(angleNext);

			Vector3 p0, p1, p2;
			if (i == 0)
			{
				p0 = Vector3(fConeLo, sinCurr * fConeRadius, cosCurr * fConeRadius);
				p1 = Vector3(fConeLo, sinNext * fConeRadius, cosNext * fConeRadius);
				p2 = Vector3(fConeHi, 0.0f, 0.0f);
			}
			else if (i == 1)
			{
				p0 = Vector3(sinCurr * fConeRadius, fConeLo, cosCurr * fConeRadius);
				p1 = Vector3(sinNext * fConeRadius, fConeLo, cosNext * fConeRadius);
				p2 = Vector3(0.0f, fConeHi, 0.0f);
			}
			else // i == 2
			{
				p0 = Vector3(sinCurr * fConeRadius, cosCurr * fConeRadius, fConeLo);
				p1 = Vector3(sinNext * fConeRadius, cosNext * fConeRadius, fConeLo);
				p2 = Vector3(0.0f, 0.0f, fConeHi);
			}

			Vector4 color = (i == 0 ? colorX : i == 1 ? colorY : colorZ);
			pSubMesh->m_vertices.push_back({ p0, color });
			pSubMesh->m_vertices.push_back({ p1, color });
			pSubMesh->m_vertices.push_back({ p2, color });

			pSubMesh->m_indices.push_back(currVertIdx);
			pSubMesh->m_indices.push_back(currVertIdx + 2);
			pSubMesh->m_indices.push_back(currVertIdx + 1);

			currVertIdx += 3;
		}

		pSubMesh->TryAddBuffers();
		pMesh->AddSubMesh(pSubMesh);
	}


	float A = 0.25f;
	float B = 0.75f;

	for (int i = 0; i < 3; i++)
	{
		UINT currVertIdx = 0;
		EditorObjectID objId = i == 0 ? EditorObjectID::TRANSLATE_XY : i == 1 ? EditorObjectID::TRANSLATE_XZ : EditorObjectID::TRANSLATE_YZ;
		NXSubMeshEditorObjects* pSubMesh = new NXSubMeshEditorObjects(pMesh, "_MoveArrows" + std::to_string(meshId++), objId);

		Vector4 color(0.8f, 0.8f, 0.7f, 0.5f);
		Vector3 p0, p1, p2, p3;
		if (i == 0)
		{
			p0 = Vector3(A, A, 0.0f);
			p1 = Vector3(A, B, 0.0f);
			p2 = Vector3(B, A, 0.0f);
			p3 = Vector3(B, B, 0.0f);
		}
		else if (i == 1)
		{
			p0 = Vector3(A, 0.0f, A);
			p1 = Vector3(A, 0.0f, B);
			p2 = Vector3(B, 0.0f, A);
			p3 = Vector3(B, 0.0f, B);
		}
		else // i == 2
		{
			p0 = Vector3(0.0f, A, A);
			p1 = Vector3(0.0f, A, B);
			p2 = Vector3(0.0f, B, A);
			p3 = Vector3(0.0f, B, B);
		}

		pSubMesh->m_vertices.push_back({ p0, color });
		pSubMesh->m_vertices.push_back({ p1, color });
		pSubMesh->m_vertices.push_back({ p2, color });
		pSubMesh->m_vertices.push_back({ p3, color });

		pSubMesh->m_indices.push_back(currVertIdx);
		pSubMesh->m_indices.push_back(currVertIdx + 2);
		pSubMesh->m_indices.push_back(currVertIdx + 1);
		pSubMesh->m_indices.push_back(currVertIdx + 1);
		pSubMesh->m_indices.push_back(currVertIdx + 2);
		pSubMesh->m_indices.push_back(currVertIdx + 3);

		currVertIdx += 4;

		pSubMesh->TryAddBuffers();
		pMesh->AddSubMesh(pSubMesh);
	}
}

const NXMeshViews& NXSubMeshGeometryEditor::GetMeshViews(const std::string& name)
{
	auto it = m_data.find(name);
	if (it != m_data.end())
	{
		return it->second;
	}
	else
	{
		return m_data["_Unknown"]; 
	}
}

void NXSubMeshGeometryEditor::Release()
{
	// m_fenceValue
	m_fenceValue++;
	m_pCommandQueue->Signal(m_pFence.Get(), m_fenceValue);

	if (m_pFence->GetCompletedValue() < m_fenceValue)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		m_pFence->SetEventOnCompletion(m_fenceValue, eventHandle);
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void NXSubMeshGeometryEditor::InitCommonMeshes()
{
	std::vector<float> verticesUnknown = { 2.0f };
	std::vector<UINT> indicesUnknown = { 0 };
	NXSubMeshGeometryEditor::GetInstance()->CreateVBIB(verticesUnknown, indicesUnknown, "_Unknown");

	float scale = 1.0f;

	// Create vertex buffer
	std::vector<VertexPT> verticesRT =
	{
		// -Z
		{ Vector3(-scale, +scale, 0.0f), Vector2(0.0f, 0.0f) },
		{ Vector3(+scale, +scale, 0.0f), Vector2(1.0f, 0.0f) },
		{ Vector3(+scale, -scale, 0.0f), Vector2(1.0f, 1.0f) },
		{ Vector3(-scale, -scale, 0.0f), Vector2(0.0f, 1.0f) },
	};

	std::vector<UINT> indicesRT =
	{
		0,  1,  2,
		0,  2,  3
	};

	NXSubMeshGeometryEditor::GetInstance()->CreateVBIB(verticesRT, indicesRT, "_RenderTarget");
}
