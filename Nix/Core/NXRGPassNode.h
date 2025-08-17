#pragma once
#include "NXRGUtil.h"
#include "NXRGResource.h"

struct NXRGResourceSlot
{
	NXRGResource* resource;
	uint32_t slot; // srv uav 的slot
	uint32_t uavCounterSlot; // uav counter 的slot（如果有uav的话）
};

class NXRGPass;
class NXRenderPass;
class NXRenderGraph;
class NXConstantBufferImpl;
class NXRGPassNodeBase
{
public:
	NXRGPassNodeBase(NXRenderGraph* pRenderGraph, const std::string& passName, NXRGPass* pPass);

	const std::string& GetName() { return m_passName; }

	// 设置Pass的根参数布局
	void SetRootParamLayout(uint32_t cbvCount, uint32_t srvCount, uint32_t uavCount);

	// 设置Pass输入的CB
	void ReadConstantBuffer(uint32_t rootIndex, uint32_t slotIndex, NXConstantBufferImpl* pConstantBuffer);

	// 设置Pass输入资源。
	void Read(NXRGResource* pResource, uint32_t passSlotIndex);

	// 设置pass输出RT。
	NXRGResource* WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool useOldVersion);
	NXRGResource* WriteDS(NXRGResource* pResource, bool useOldVersion);
	NXRGResource* WriteUAV(NXRGResource* pResource, uint32_t uavIndex, bool useOldVersion, uint32_t uavCounterIndex);
	NXRGResource* SetIndirectArgs(NXRGResource* pResource);

	NXRenderPass* GetRenderPass();

	void Compile(bool isResize);
	virtual void Execute(ID3D12GraphicsCommandList* pCmdList) = 0;

	// 获取pass关联资源
	const std::vector<NXRGResourceSlot>& GetInputs() { return m_inputs; }
	const std::vector<NXRGResourceSlot>& GetOutputs() { return m_outputs; }

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
	std::vector<NXRGResourceSlot> m_inputs; 
	std::vector<NXRGResourceSlot> m_outputs;
	NXRGResource* m_indirectArgs;

	// 记录当前pass的根参数布局
	NXRGRootParamLayout m_rootParamLayout; 
};

template<typename NXRGPassData>
class NXRGPassNode : public NXRGPassNodeBase
{
public:
	NXRGPassNode(NXRenderGraph* pRenderGraph, const std::string& passName, NXRGPass* pPass) : NXRGPassNodeBase(pRenderGraph, passName, pPass), m_passData(NXRGPassData()) {}

	NXRGPassData& GetData() { return m_passData; }

	void Execute(ID3D12GraphicsCommandList* pCmdList) override
	{
		if (!m_pPassInited)
		{
			m_pPass->SetupInternal();
			m_pPassInited = true;
		}

		NX12Util::BeginEvent(pCmdList, m_passName.c_str());

		m_executeFunc(pCmdList, m_passData);
		m_pPass->Render(pCmdList);

		NX12Util::EndEvent(pCmdList);
	}

	void RegisterExecuteFunc(std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> func) { m_executeFunc = func; }

private:
	NXRGPassData m_passData;

	// 如果需要在执行前进行一些pass操作（比如clearRT），可使用此方法的lambda。
	std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> m_executeFunc;
};
