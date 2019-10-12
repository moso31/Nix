#include "NXMesh.h"
#include "FBXMeshLoader.h"

NXMesh::NXMesh()
{
}

NXMesh::~NXMesh()
{
}

void NXMesh::Init(string filePath)
{
	FBXMeshLoader::LoadFBXFile(filePath, this);

	m_filePath = filePath;

	InitVertexIndexBuffer();
	InitAABB();
}
