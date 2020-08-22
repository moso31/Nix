#pragma once
#include "NXPrimitive.h"

class NXMesh : public NXPrimitive
{
public:
	NXMesh();
	~NXMesh();

	void Init(std::string filePath = "");

private:
	std::string m_filePath;

	friend class FBXMeshLoader;
};
