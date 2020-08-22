#pragma once
#include <memory>
#include <string>
#include <vector>

#include "Common.h"

class NXMesh;
class NXScene;

class FBXMeshLoader
{
public:
	static void LoadContent(FbxNode* pNode, std::shared_ptr<NXMesh>& pEngineMesh, std::vector<std::shared_ptr<NXMesh>>& outMeshes);
	static void LoadNodeTransformInfo(FbxNode* pNode, std::shared_ptr<NXMesh>& pEngineMesh);

	static void LoadMesh(FbxNode* pNode, std::shared_ptr<NXMesh>& pEngineMesh);
	static void LoadPolygons(FbxMesh* pMesh, std::shared_ptr<NXMesh>& pEngineMesh);

	static void LoadFBXFile(std::string filepath, std::shared_ptr<NXScene> pRenderScene, std::vector<std::shared_ptr<NXMesh>>& outMeshes);
};