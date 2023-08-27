#include "NXScriptResourceManager.h"
#include "NXScriptable.h"
#include "NXScene.h"
#include "NXScriptType.h"
#include "NSFirstPersonalCamera.h"

NXScript* NXScriptResourceManager::CreateScript(const NXScriptType scriptType, NXScriptable* pObject)
{
	switch (scriptType)
	{
	case NXScriptType::NXSCRIPT_FIRST_PERSONAL_CAMERA:
	{
		auto pScript = new NSFirstPersonalCamera();
		pObject->AddScript(pScript);
		//m_scripts.push_back(pScript);
		return pScript;
	}
	default:
		return nullptr;
	}
}

void NXScriptResourceManager::OnReload()
{
}

void NXScriptResourceManager::Release()
{
}
