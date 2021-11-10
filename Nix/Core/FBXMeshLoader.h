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

class NXScene;
class FBXMeshLoader
{
public:
	static void LoadContent(FbxNode* pNode, NXPrimitive* pEngineMesh, std::vector<NXPrimitive*>& outMeshes, bool bAutoCalcTangents);
	static void LoadNodeTransformInfo(FbxNode* pNode, NXPrimitive* pEngineMesh);

	static void LoadMesh(FbxNode* pNode, NXPrimitive* pEngineMesh, bool bAutoCalcTangents);
	static void LoadPolygons(FbxMesh* pMesh, NXPrimitive* pEngineMesh, int lSubMeshCount, bool bAutoCalcTangents);

	static void LoadFBXFile(std::string filepath, NXScene* pRenderScene, std::vector<NXPrimitive*>& outMeshes, bool bAutoCalcTangents);

private:
	static void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
	static bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);
};