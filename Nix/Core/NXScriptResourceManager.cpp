#include "NXScriptResourceManager.h"
#include "NXScriptable.h"
#include "NXScene.h"
#include "NXScriptType.h"
#include "NSFirstPersonalCamera.h"

NXScript* NXScriptResourceManager::CreateScript(const NXScriptType scriptType, NXScriptable* pObject)
{
	// NXScript的内存生命周期由 NXObject 管理，而不用此Manager管理
	// 因为每个对象的脚本都是独立的
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
