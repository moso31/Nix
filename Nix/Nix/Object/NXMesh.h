#pragma once
#include "NXPrimitive.h"

class NXMesh : public NXPrimitive
{
public:
	NXMesh();
	~NXMesh();

	void Init(string filePath = "");
	void Render();
	void Release();

private:
	string m_filePath;

	friend class FBXMeshLoader;
};
