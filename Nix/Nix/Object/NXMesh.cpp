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
	m_filePath = filePath;

	InitVertexIndexBuffer();
	InitAABB();
}
