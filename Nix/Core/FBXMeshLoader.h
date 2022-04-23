#pragma once
#include "header.h"

#if defined(DEBUG) | defined(_DEBUG)
	#undef DEBUG_NEW
	#undef new
#endif

#include <fbxsdk.h>

#if defined(DEBUG) | defined(_DEBUG)
    #define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__)
    #define new DEBUG_NEW 
#endif

struct FBXMeshVertexData
{
	FBXMeshVertexData() = default;
	~FBXMeshVertexData() {}

	Vector3 Position;
	std::vector<Vector4> VertexColors;
	std::vector<Vector2> UVs;
	std::vector<Vector3> Normals;
	std::vector<Vector3> Tangents;
	std::vector<Vector3> BiTangents;
};

struct VertexPNTT;

class NXScene;
class FBXMeshLoader
{
public:
	static void LoadFBXFile(std::string filepath, NXPrefab* pOutPrefab, bool bAutoCalcTangents);
	static void LoadRenderableObjects(FbxNode* pNode, NXRenderableObject* pParentMesh, bool bAutoCalcTangents);

private:
	static void EncodePrimitiveData(FbxNode* pNode, NXPrimitive* pPrimitive, bool bAutoCalcTangents);
	static void EncodePolygonData(FbxMesh* pMesh, NXSubMesh* pSubMesh, int polygonIndex, int& vertexId);
	static void EncodeVertexPosition(FBXMeshVertexData& inoutVertexData, FbxMesh* pMesh, FbxVector4* pControlPoints, int controlPointIndex);
	static void EncodeVertexColors(FBXMeshVertexData& inoutVertexData, FbxMesh* pMesh, int controlPointIndex, int vertexId);
	static void EncodeVertexUVs(FBXMeshVertexData& inoutVertexData, FbxMesh* pMesh, int polygonIndex, int polygonVertexIndex, int controlPointIndex, int vertexId);
	static void EncodeVertexNormals(FBXMeshVertexData& inoutVertexData, FbxMesh* pMesh, int vertexId);
	static void EncodeVertexTangents(FBXMeshVertexData& inoutVertexData, FbxMesh* pMesh, int vertexId);
	static void EncodeVertexBiTangents(FBXMeshVertexData& inoutVertexData, FbxMesh* pMesh, int vertexId);

	static void ConvertVertexFormat(FBXMeshVertexData inVertexData, VertexPNTT& outVertexData);

private:
	static void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
	static bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);
};