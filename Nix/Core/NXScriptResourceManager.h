#pragma once
#include "NXResourceManagerBase.h"

enum NXScriptType;
class NXScriptResourceManager : public NXResourceManagerBase
{
public:
	NXScriptResourceManager() {}
	~NXScriptResourceManager() {}

	static NXScript* CreateScript(const NXScriptType scriptType, NXObject* pObject);

	void OnReload() override;
	void Release() override;
};