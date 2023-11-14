#pragma once
#include <vector>
#include <queue>
#include <any>
#include "NXInstance.h"

enum NXGUICommandType
{
	NXGUICmd_None,
	NXGUICmd_Inspector_SetIdx,
	NXGUICmd_Inspector_OpenShaderEditor,
	NXGUICmd_MSE_SetMaterial,
	NXGUICmd_MSE_CompileSuccess,
};

using NXGUICommandArg = std::any;

struct NXGUICommand
{
	NXGUICommand() = default;
	NXGUICommand(NXGUICommandType type) : type(type) {}
	NXGUICommand(NXGUICommandType type, const std::vector<NXGUICommandArg>& args) : type(type), args(args) {}

	NXGUICommandType type;
	std::vector<NXGUICommandArg> args;
};

class NXGUIInspector;
class NXGUICommandManager : public NXInstance<NXGUICommandManager>
{
public:
	void Init(NXGUIInspector* pGUIInspector);

	void Update();

	void PushCommand(const NXGUICommand& cmd);
	
private:
	NXGUIInspector* m_pGUIInspector;
	std::queue<NXGUICommand> m_pCommandQueue;
};
