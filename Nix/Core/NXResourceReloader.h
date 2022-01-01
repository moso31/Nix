#pragma once
#include "NXInstance.h"

struct NXResourceReloadCommand {};

struct NXResourceReloadCubeMapCommand : public NXResourceReloadCommand
{
	NXCubeMap* pCubeMap;
	std::wstring strFilePath;
};

class NXResourceReloader : public NXInstance<NXResourceReloader>
{
public:
	NXResourceReloader() {}
	~NXResourceReloader() {}

	void Push(NXResourceReloadCommand* pCommand);

	void Update();

private:
	std::vector<NXResourceReloadCommand*> m_resourceReloadCmdList;
};