#pragma once
#include "NXResourceManagerBase.h"

enum NXScriptType;
class NXScript;
class NXScriptable;
class NXScriptResourceManager : public NXResourceManagerBase
{
public:
	NXScriptResourceManager() {}
	~NXScriptResourceManager() {}

	static NXScript* CreateScript(const NXScriptType scriptType, NXScriptable* pObject);

	void OnReload() override;
	void Release() override;
};