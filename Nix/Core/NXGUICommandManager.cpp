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
		case NXGUICmd_Inspector_SetTexture:
			m_pGUIInspector->DoCommand(e);
		default:
			break;
		}
	}
}

void NXGUICommandManager::PushCommand(const NXGUICommand& cmd)
{
	m_pCommandQueue.push(cmd);
}
