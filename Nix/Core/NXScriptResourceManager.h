#pragma once
#include "NXResourceManagerBase.h"

enum NXScriptType;
class NXScript;
class NXScriptable;
class NXScriptResourceManager : public NXResourceManagerBase
{
public:
	NXScriptResourceManager() {}
	virtual ~NXScriptResourceManager() {}

	static NXScript* CreateScript(const NXScriptType scriptType, NXScriptable* pObject);

	void OnReload() override;
	void Release() override;
};