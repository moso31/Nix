#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Common.h"

using namespace std;

class NXMesh;
class NXScene;

class FBXMeshLoader
{
public:
	static void LoadContent(FbxNode* pNode, shared_ptr<NXMesh>& pEngineMesh, vector<shared_ptr<NXMesh>>& outMeshes);
	static void LoadNodeTransformInfo(FbxNode* pNode, shared_ptr<NXMesh>& pEngineMesh);

	static void LoadMesh(FbxNode* pNode, shared_ptr<NXMesh>& pEngineMesh);
	static void LoadPolygons(FbxMesh* pMesh, shared_ptr<NXMesh>& pEngineMesh);

	static void LoadFBXFile(string filepath, shared_ptr<NXScene> pRenderScene, vector<shared_ptr<NXMesh>>& outMeshes);
};