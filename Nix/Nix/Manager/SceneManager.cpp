#include "SceneManager.h"
#include "NSFirstPersonalCamera.h"

SceneManager::SceneManager()
{
}

SceneManager::SceneManager(const shared_ptr<Scene>& pScene) :
	m_scene(pScene)
{
}

SceneManager::~SceneManager()
{
}

shared_ptr<NXScript> SceneManager::CreateScript(const NXScriptType scriptType, const shared_ptr<NXObject>& pObject)
{
	switch (scriptType)
	{
	case NXScriptType::NXSCRIPT_FIRST_PERSONAL_CAMERA:
	{
		auto pScript = make_shared<NSFirstPersonalCamera>();
		//m_scene->GetScripts().push_back(pScript);
		return pScript;
	}
	default:
		return nullptr;
	}
}

shared_ptr<NXListener> SceneManager::AddEventListener(const NXEventType eventType, const shared_ptr<NXObject>& pObject, const function<void(NXEventArg)>& pFunc)
{
	auto pListener = make_shared<NXListener>(pObject, pFunc);
	switch (eventType)
	{
	case NXEventType::NXEVENT_KEYDOWN:
	{
		NXEventKeyDown::GetInstance()->AddListener(pListener);
		break;
	}
	case NXEventType::NXEVENT_KEYUP:
	{
		NXEventKeyUp::GetInstance()->AddListener(pListener);
		break;
	}
	case NXEventType::NXEVENT_MOUSEDOWN:
	{
		NXEventMouseDown::GetInstance()->AddListener(pListener);
		break;
	}
	case NXEventType::NXEVENT_MOUSEUP:
	{
		NXEventMouseUp::GetInstance()->AddListener(pListener);
		break;
	}
	case NXEventType::NXEVENT_MOUSEMOVE:
	{
		NXEventMouseMove::GetInstance()->AddListener(pListener);
		break;
	}
	default:
		return nullptr;
	}
	return pListener;
}
