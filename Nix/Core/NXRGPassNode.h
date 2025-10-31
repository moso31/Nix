#pragma once
#include "NXRGUtil.h"
#include "NXRGResource.h"
#include "NXRGBuilder.h"

struct NXRGResourceSlotSpace
{
	NXRGResource* resource;
	uint32_t slot = (uint32_t)-1; // srv uav 的slot
	uint32_t uavCounterSlot = (uint32_t)-1; // uav counter 的slot（如果有uav的话）
	uint32_t space = 0; // srv uav 的space
	uint32_t uavCounterSpace = 0; // uav counter 的space（如果有uav的话）
};

class NXRGPass;
class NXRenderPass;
class NXRenderGraph;
class NXConstantBufferImpl;
class NXReadbackData;
class NXRGPassNodeBase
{
public:
	NXRGPassNodeBase(NXRenderGraph* pRenderGraph, const std::string& passName, NXRGPass* pPass);

	const std::string& GetName() { return m_passName; }

	// 设置Pass输入的CB
	// 【TODO：暂时可传nullptr，表示给此CBV预留槽位，但不由NXRG指定。由于传统多Pass写法尚未完全解耦，故作此妥协】
	void ReadConstantBuffer(uint32_t rootIndex, uint32_t slotIndex, uint32_t spaceIndex, NXConstantBufferImpl* pConstantBuffer);

	// 设置Pass输入资源。
	void Read(NXRGResource* pResource, uint32_t passSlotIndex);

	// 设置pass输出RT。
	NXRGResource* WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool useOldVersion);
	NXRGResource* WriteDS(NXRGResource* pResource, bool useOldVersion);
	NXRGResource* WriteUAV(NXRGResource* pResource, uint32_t uavIndex, bool useOldVersion);
	NXRGResource* WriteUAVCounter(NXRGResource* pResource, uint32_t uavCounterIndex);

	// 设置Pass的间接参数
	NXRGResource* SetIndirectArgs(NXRGResource* pResource);

	// 设置Pass输入到哪个回读data（仅回读Pass使用）
	void WriteReadbackData(Ntr<NXReadbackData>& data) { m_readbackData = data; }

	NXRenderPass* GetRenderPass();

	virtual void Setup(NXRGBuilder& builder) = 0;
	void Compile(bool isResize);
	virtual void Execute(ID3D12GraphicsCommandList* pCmdList) = 0;

	// 获取pass关联资源
	const std::vector<NXRGResourceSlotSpace>& GetInputs() { return m_inputs; }
	const std::vector<NXRGResourceSlotSpace>& GetOutputs() { return m_outputs; }

	void SetCommandContext(const NXRGCommandContext& ctx);

private:
	void Compile_GraphicsPass(bool isResize);
	void Compile_ComputePass(bool isResize);
	void Compile_ReadbackBufferPass();

protected:
	std::string m_passName;
	NXRenderGraph* m_pRenderGraph;

	NXRGPass* m_pPass;
	bool m_pPassInited;

	// Pass记录自己依赖的资源指针（但不负责其生命周期）
	std::vector<NXRGResourceSlotSpace> m_inputs; 
	std::vector<NXRGResourceSlotSpace> m_outputs;

	// Pass是否使用间接参数作为draw/dispatch
	NXRGResource* m_indirectArgs;

	// 特化：如果是回读Pass，使用这个数据存回读Pass的data
	// 【要做成NXRGResource节点也可以，但要改至少NXResource一级的继承结构，现阶段不是很划得来】
	Ntr<NXReadbackData> m_readbackData;
};

template<typename NXRGPassData>
class NXRGPassNode : public NXRGPassNodeBase
{
public:
	NXRGPassNode(NXRenderGraph* pRenderGraph, const std::string& passName, NXRGPass* pPass) : NXRGPassNodeBase(pRenderGraph, passName, pPass), m_passData(NXRGPassData()) {}

	NXRGPassData& GetData() { return m_passData; }

	void Setup(NXRGBuilder& builder) override
	{
		m_setupFunc(builder, m_passData);
	}

	void Execute(ID3D12GraphicsCommandList* pCmdList) override
	{
		if (!m_pPassInited)
		{
			m_pPass->SetPassName(m_passName);
			m_pPass->SetupInternal();
			m_pPassInited = true;
		}

		NX12Util::BeginEvent(pCmdList, m_passName.c_str());

		m_executeFunc(pCmdList, m_passData);
		m_pPass->Render();

		NX12Util::EndEvent(pCmdList);
	}

	void RegisterSetupFunc(std::function<void(NXRGBuilder& pBuilder, NXRGPassData& data)> func) { m_setupFunc = std::move(func); }
	void RegisterExecuteFunc(std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> func) { m_executeFunc = std::move(func); }

private:
	NXRGPassData m_passData;

	std::function<void(NXRGBuilder& pBuilder, NXRGPassData& data)> m_setupFunc;
	std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> m_executeFunc;
};
