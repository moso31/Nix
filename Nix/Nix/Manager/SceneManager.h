#pragma once
#include "NXScene.h"
#include "NXScriptType.h"
#include "NXEvent.h"

class SceneManager
{
public:
	SceneManager();
	SceneManager(const shared_ptr<Scene>& pScene);
	~SceneManager();

	shared_ptr<NXScript>	CreateScript(const NXScriptType scriptType, const shared_ptr<NXObject>& pObject);
	shared_ptr<NXListener>	AddEventListener(const NXEventType eventType, const shared_ptr<NXObject>& pObject, const function<void(NXEventArg)>& pFunc);

private:
	shared_ptr<Scene> m_scene;
};
