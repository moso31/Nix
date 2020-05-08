#pragma once
#include "NXPrimitive.h"

class NXMesh : public NXPrimitive
{
public:
	NXMesh();
	~NXMesh();

	void Init(string filePath = "");

private:
	string m_filePath;

	friend class FBXMeshLoader;
};
