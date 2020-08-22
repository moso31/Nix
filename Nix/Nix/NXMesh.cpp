#include "NXMesh.h"
#include "FBXMeshLoader.h"

NXMesh::NXMesh()
{
}

NXMesh::~NXMesh()
{
}

void NXMesh::Init(std::string filePath)
{
	m_filePath = filePath;

	InitVertexIndexBuffer();
	InitAABB();
}
