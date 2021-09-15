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

class NXMesh;
class NXScene;
class FBXMeshLoader
{
public:
	static void LoadContent(FbxNode* pNode, NXMesh* pEngineMesh, std::vector<NXMesh*>& outMeshes, bool bAutoCalcTangents);
	static void LoadNodeTransformInfo(FbxNode* pNode, NXMesh* pEngineMesh);

	static void LoadMesh(FbxNode* pNode, NXMesh* pEngineMesh);
	static void LoadPolygons(FbxMesh* pMesh, NXMesh* pEngineMesh);

	static void LoadFBXFile(std::string filepath, NXScene* pRenderScene, std::vector<NXMesh*>& outMeshes, bool bAutoCalcTangents);

private:
	static void InitializeSdkObjects(FbxManager*& pManager, FbxScene*& pScene);
	static bool LoadScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename);
};