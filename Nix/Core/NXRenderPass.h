#pragma once
#include "BaseDefs/DX12.h"
#include <filesystem>
#include "Ntr.h"
#include "NXCommonTexDefinition.h"
#include "NXTexture.h"

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

class NXRenderPass
{
public:
	NXRenderPass();
	virtual ~NXRenderPass() {}

	virtual void SetupInternal() = 0;

	void SetPassName(const std::string& passName) { m_passName = passName; }

protected:
	std::string	m_passName;
};
