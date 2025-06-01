#pragma once
#include "BaseDefs/DX12.h"
#include "NXRendererPass.h"
#include "NXBuffer.h"

class NXComputePass : public NXRendererPass
{
public:
	NXComputePass();
	virtual ~NXComputePass() {}

	virtual void SetupInternal() = 0;

	// ��ǰ Nix Compute Pass �ĸ��������Ͳ�������-�Ĵ����Ĳ��ֹ���
	// 1. ÿ��CBVռ��һ��������
	// 2. ������SRV������ЩSRV�����ŵ�һ��������Table���ռ��һ����������
	// 3. ������UAV������ЩUAVҲ���ŵ�һ��������Table���ռ��һ����������
	// ��2.3. SRV������UAVǰ�棩
	// 4. �κ�����¶���ʹ�ø�����
	// 5. ������ʼ��ʹ��StaticSampler�������Ƕ�̬Sampler��Ŀǰ������
	void SetRootParams(int CBVNum, int SRVNum, int UAVNum);

	void SetInputTex(NXCommonTexEnum eCommonTex, uint32_t slotIndex);
	void SetInputTex(const Ntr<NXTexture>& pTex, uint32_t slotIndex);
	void SetOutputTex(const Ntr<NXTexture>& pTex, uint32_t slotIndex);
	void SetInputBuffer(const Ntr<NXBuffer>& pBuffer, uint32_t slotIndex);
	void SetOutputBuffer(const Ntr<NXBuffer>& pBuffer, uint32_t slotIndex);

private:
	D3D12_COMPUTE_PIPELINE_STATE_DESC m_csoDesc;

	std::vector<Ntr<NXResource>>			m_pInRes;
	std::vector<Ntr<NXResource>>			m_pOutRes;

	// pass ʹ�õĸ�����
	std::vector<D3D12_ROOT_PARAMETER>		m_rootParams;

	// pass ʹ�õ� srv/uav ��������
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_srvRanges;
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_uavRanges;

	// pass ʹ�õľ�̬������
	std::vector<D3D12_STATIC_SAMPLER_DESC>	m_staticSamplers;

	// Pass������Ҫ�������������Pass������ЩCB��
	std::vector<NXCBVManagement>			m_cbvManagements;
};