#pragma once
#include "NXRGUtil.h"
#include "NXRGResource.h"

struct NXRGResourceSlot
{
	NXRGResource* resource;
	uint32_t slot; // srv uav ��slot
	uint32_t uavCounterSlot; // uav counter ��slot�������uav�Ļ���
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

	// ����Pass�ĸ���������
	void SetRootParamLayout(uint32_t cbvCount, uint32_t srvCount, uint32_t uavCount);

	// ����Pass�����CB
	void ReadConstantBuffer(uint32_t rootIndex, uint32_t slotIndex, NXConstantBufferImpl* pConstantBuffer);

	// ����Pass������Դ��
	void Read(NXRGResource* pResource, uint32_t passSlotIndex);

	// ����pass���RT��
	NXRGResource* WriteRT(NXRGResource* pResource, uint32_t outRTIndex, bool useOldVersion);
	NXRGResource* WriteDS(NXRGResource* pResource, bool useOldVersion);
	NXRGResource* WriteUAV(NXRGResource* pResource, uint32_t uavIndex, bool useOldVersion, uint32_t uavCounterIndex);
	NXRGResource* SetIndirectArgs(NXRGResource* pResource);

	NXRenderPass* GetRenderPass();

	void Compile(bool isResize);
	virtual void Execute(ID3D12GraphicsCommandList* pCmdList) = 0;

	// ��ȡpass������Դ
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

	// Pass��¼�Լ���������Դָ�루�����������������ڣ�
	std::vector<NXRGResourceSlot> m_inputs; 
	std::vector<NXRGResourceSlot> m_outputs;
	NXRGResource* m_indirectArgs;

	// ��¼��ǰpass�ĸ���������
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

	// �����Ҫ��ִ��ǰ����һЩpass����������clearRT������ʹ�ô˷�����lambda��
	std::function<void(ID3D12GraphicsCommandList* pCmdList, NXRGPassData& data)> m_executeFunc;
};
