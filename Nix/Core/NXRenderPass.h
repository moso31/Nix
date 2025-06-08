#pragma once
#include "BaseDefs/DX12.h"
#include <filesystem>
#include "Ntr.h"
#include "NXCommonTexDefinition.h"
#include "NXTexture.h"

struct NXRGRootParamLayout
{
	int cbvCount = 0;
	int srvCount = 0;
	int uavCount = 0;
};

// ��DX12Ҫ��CB����Ҫ�ṩ��ӦCBV��gpuHandle��
// cmdList��ʹ��gpuHandle��
struct NXCBVManagement
{
	// ���ڼ�¼ÿ֡ cmdList��ν��� cbv gpu �����ַ��
	// true: ʹ�� multiFrameGpuVirtAddr �еĵ�ַ��
	// false: �������ֶ����£����ﲻ�ùܡ�
	bool autoUpdate = false;

	// �������autoUpdate��ʹ�������gpuHandle��D3D12_GPU_VIRTUAL_ADDRESS����
	const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* multiFrameGpuVirtAddr;
};

enum class NXRenderPassType
{
	GraphicPass,
	ComputePass,
};

class NXRenderPass
{
public:
	NXRenderPass(NXRenderPassType type);
	virtual ~NXRenderPass() {}

	virtual void SetupInternal() = 0;

	NXRenderPassType GetPassType() const { return m_passType; }

	void SetPassName(const std::string& passName) { m_passName = passName; }
	void SetShaderFilePath(const std::filesystem::path& shaderFilePath) { m_shaderFilePath = shaderFilePath; }

	void SetEntryNameVS(const std::wstring& name) { m_entryNameVS = name; }
	void SetEntryNamePS(const std::wstring& name) { m_entryNamePS = name; }
	void SetEntryNameCS(const std::wstring& name) { m_entryNameCS = name; }

	virtual void Render(ID3D12GraphicsCommandList* pCmdList) = 0;

	// ��ǰ Nix �ĸ��������Ͳ�������-�Ĵ����Ĳ��ֹ���
	// 1. ÿ��CBVռ��һ��������
	// 2. ������SRV������ЩSRV�����ŵ�һ��������Table���ռ��һ����������
	// 3. ������UAV������ЩUAVҲ���ŵ�һ��������Table���ռ��һ����������
	// ��2.3. SRV������UAVǰ�棩
	// 4. �κ�����¶���ʹ�ø�����
	// 5. ������ʼ��ʹ��StaticSampler�������Ƕ�̬Sampler��Ŀǰ������
	virtual void SetRootParamLayout(const NXRGRootParamLayout& layout);

	// ����CBV��
	// rootParamIndex: ������������
	// slotIndex: ���������������������ṩ�����rootParamIndex��ͬ��
	// gpuVirtAddr: CBV��gpu�����ַ
	void SetStaticRootParamCBV(int rootParamIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs);
	void SetStaticRootParamCBV(int rootParamIndex, int slotIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs);

	void AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& staticSampler);
	void AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW);

protected:
	std::string	m_passName;
	NXRenderPassType m_passType;
	std::filesystem::path m_shaderFilePath;

	// shader��ڵ�����
	std::wstring m_entryNameVS;
	std::wstring m_entryNamePS;
	std::wstring m_entryNameCS;

	// pass ʹ�õľ�̬������
	std::vector<D3D12_STATIC_SAMPLER_DESC>	m_staticSamplers;

	// pass ʹ�õĸ�����+����
	std::vector<D3D12_ROOT_PARAMETER> m_rootParams;
	NXRGRootParamLayout m_rootParamLayout;

	// ������ǰpass cb�����벼��
	std::vector<NXCBVManagement> m_cbvManagements;

	// pass ʹ�õ� srv/uav ��������
	// ע��Nix�� srv = ���룬uav = �����uav ��ʱ��֧�����롣
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_srvRanges;
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_uavRanges;
};
