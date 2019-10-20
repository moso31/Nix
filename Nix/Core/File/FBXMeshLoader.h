#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Common.h"

using namespace std;

class NXMesh;
class Scene;

class FBXMeshLoader
{
public:
	static void LoadContent(FbxScene* pScene, shared_ptr<NXMesh> pEngineMesh);
	static void LoadContent(FbxNode* pNode, shared_ptr<NXMesh> pEngineMesh);
	static void LoadNodeTransformInfo(FbxNode* pNode, shared_ptr<NXMesh> pEngineMesh);

	static void LoadMesh(FbxNode* pNode, shared_ptr<NXMesh> pEngineMesh);
	static void LoadPolygons(FbxMesh* pMesh, shared_ptr<NXMesh> pEngineMesh);

	static void LoadFBXFile(string filepath, shared_ptr<Scene> pRenderScene, vector<shared_ptr<NXMesh>>& outMeshes);
};