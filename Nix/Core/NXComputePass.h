#pragma once
#include "NXRenderPass.h"
#include "NXBuffer.h"

class NXComputePass : public NXRenderPass
{
public:
	NXComputePass();
	virtual ~NXComputePass() {}

	virtual void SetupInternal() = 0;

	void InitCSO();

	void SetThreadGroups(uint32_t threadGroupX, uint32_t threadGroupY = 1, uint32_t threadGroupZ = 1);

	void SetInputTex(NXCommonTexEnum eCommonTex, uint32_t slotIndex);
	void SetInputTex(const Ntr<NXTexture>& pTex, uint32_t slotIndex);
	void SetOutputTex(const Ntr<NXTexture>& pTex, uint32_t slotIndex);
	void SetInputBuffer(const Ntr<NXBuffer>& pBuffer, uint32_t slotIndex);
	void SetOutputBuffer(const Ntr<NXBuffer>& pBuffer, uint32_t slotIndex);

	virtual void RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList);
	virtual void RenderBefore(ID3D12GraphicsCommandList* pCmdList);
	virtual void Render(ID3D12GraphicsCommandList* pCmdList);

	// ��ǰ Nix Compute Pass �ĸ��������Ͳ�������-�Ĵ����Ĳ��ֹ���
	// 1. ÿ��CBVռ��һ��������
	// 2. ������SRV������ЩSRV�����ŵ�һ��������Table���ռ��һ����������
	// 3. ������UAV������ЩUAVҲ���ŵ�һ��������Table���ռ��һ����������
	// ��2.3. SRV������UAVǰ�棩
	// 4. �κ�����¶���ʹ�ø�����
	// 5. ������ʼ��ʹ��StaticSampler�������Ƕ�̬Sampler��Ŀǰ������
	void SetRootParams(int CBVNum, int SRVNum, int UAVNum);

	// ����CBV��
	// rootParamIndex: ������������
	// slotIndex: ���������������������ṩ�����rootParamIndex��ͬ��
	// gpuVirtAddr: CBV��gpu�����ַ
	void SetStaticRootParamCBV(int rootParamIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs);
	void SetStaticRootParamCBV(int rootParamIndex, int slotIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs);

	void AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& staticSampler);
	void AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW);

private:
	D3D12_COMPUTE_PIPELINE_STATE_DESC		m_csoDesc;
	ComPtr<ID3D12PipelineState>				m_pCSO;
	ComPtr<ID3D12RootSignature>				m_pRootSig;

	uint32_t 								m_threadGroupX;
	uint32_t 								m_threadGroupY;
	uint32_t 								m_threadGroupZ;

	std::vector<Ntr<NXResource>>			m_pInRes;
	std::vector<Ntr<NXResource>>			m_pOutRes;

	std::filesystem::path					m_shaderFilePath;

	// pass ʹ�õĸ�����
	std::vector<D3D12_ROOT_PARAMETER>		m_rootParams;

	// pass ʹ�õ� srv/uav ��������
	// ע��Nix�� srv = ���룬uav = �����uav ��ʱ��֧�����롣
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_srvRanges; 
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_uavRanges;

	// pass ʹ�õľ�̬������
	std::vector<D3D12_STATIC_SAMPLER_DESC>	m_staticSamplers;

	// Pass������Ҫ�������������Pass������ЩCB��
	std::vector<NXCBVManagement>			m_cbvManagements;
};