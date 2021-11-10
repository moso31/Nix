#pragma once
#include "ShaderStructures.h"

enum NXPlaneAxis
{
	POSITIVE_X,
	POSITIVE_Y,
	POSITIVE_Z,
	NEGATIVE_X,
	NEGATIVE_Y,
	NEGATIVE_Z
};

class NXSubMeshGeometryEditor
{
public:
	NXSubMeshGeometryEditor();
	~NXSubMeshGeometryEditor();

	static void CreateFBXMesh(NXPrimitive* pMesh, UINT subMeshCount, VertexPNTT** pSubMeshVertices, UINT* pSubMeshVerticesCounts, UINT** pSubMeshIndices, UINT* pSubMeshIndicesCounts, bool bAutoCalcTangents);

	static void CreateBox(NXPrimitive* pMesh, float x = 1.0f, float y = 1.0f, float z = 1.0f);
	static void CreateCylinder(NXPrimitive* pMesh, float radius = 1.0f, float length = 3.0f, int segmentCircle = 16, int segmentLength = 4);
	static void CreatePlane(NXPrimitive* pMesh, float width = 0.5f, float height = 0.5f, NXPlaneAxis Axis = POSITIVE_Y);
	static void CreateSphere(NXPrimitive* pMesh, float radius = 1.0f, int segmentHorizontal = 16, int segmentVertical = 16);

private:
};
