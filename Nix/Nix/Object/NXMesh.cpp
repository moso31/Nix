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

	for (int i = 0; i < m_vertices.size(); i++)
	{
		//m_aabb.Merge(m_vertices[i].pos);
	}

	m_filePath = filePath;

	InitVertexIndexBuffer();
	InitAABB();
}
