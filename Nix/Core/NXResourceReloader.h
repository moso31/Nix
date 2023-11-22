#pragma once
#include <string>
#include <vector>
#include "NXInstance.h"

struct NXResourceReloadCommand {};

class NXCubeMap;
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

	void OnReload();

private:

private:
	std::vector<NXResourceReloadCommand*> m_resourceReloadCmdList;
};