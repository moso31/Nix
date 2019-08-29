#pragma once
#include <memory>
#include <string>

#include "Common.h"

using namespace std;

class NXMesh;

class FBXMeshLoader
{
public:
	static void LoadContent(FbxScene* pScene, NXMesh* pEngineMesh);
	static void LoadContent(FbxNode* pNode, NXMesh* pEngineMesh);
	static void DisplayGeometricTransform(FbxNode* pNode, NXMesh* pEngineMesh);

	static void LoadMesh(FbxNode* pNode, NXMesh* pEngineMesh);
	static void LoadPolygons(FbxMesh* pMesh, NXMesh* pEngineMesh);

	static void LoadFBXFile(string filepath, NXMesh* pEngineMesh);
};