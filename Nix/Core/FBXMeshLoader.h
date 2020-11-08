#pragma once
#include "header.h"
#include "Common.h"

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
};