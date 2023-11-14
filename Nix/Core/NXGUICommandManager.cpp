#include "NXGUICommandManager.h"
#include "NXGUIInspector.h"

void NXGUICommandManager::Init(NXGUIInspector* pGUIInspector)
{
	m_pGUIInspector = pGUIInspector;
}

void NXGUICommandManager::Update()
{
	while (!m_pCommandQueue.empty())
	{
		auto e = m_pCommandQueue.front();
		m_pCommandQueue.pop();

		switch (e.type)
		{
		case NXGUICmd_Inspector_SetIdx:
		case NXGUICmd_Inspector_OpenShaderEditor:
		case NXGUICmd_MSE_SetMaterial:
		case NXGUICmd_MSE_CompileSuccess:
			m_pGUIInspector->DoCommand(e);
			break;
		default:
			break;
		}
	}
}

void NXGUICommandManager::PushCommand(const NXGUICommand& cmd)
{
	m_pCommandQueue.push(cmd);
}
