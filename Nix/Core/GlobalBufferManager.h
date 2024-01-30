#pragma once
#include "BaseDefs/DX12.h"
#include "CommittedAllocator.h"
#include "ShaderStructures.h"

// 2024.1.26 TODO��cbData��Object��View�ֿ��ԣ������ڵķַ������⡣
// m_cbDataObject�д���һЩview proj֮��Ĳ�����ʵ����Ӧ�÷���Camera�С� // �����ٸģ�����û��
class NXGlobalBufferManager
{
public:
	static void Init();

	static MultiFrame<CommittedResourceData<ConstantBufferObject>>		m_cbDataObject;
	static MultiFrame<CommittedResourceData<ConstantBufferCamera>>		m_cbDataCamera;
	static MultiFrame<CommittedResourceData<ConstantBufferShadowTest>>	m_cbDataShadowTest;
};

class NXGlobalInputLayout
{
public:
	static void Init();

	static D3D12_INPUT_ELEMENT_DESC	layoutP[1];
	static D3D12_INPUT_ELEMENT_DESC	layoutPT[2];
	static D3D12_INPUT_ELEMENT_DESC	layoutPNT[3];
	static D3D12_INPUT_ELEMENT_DESC	layoutPNTT[4];
	static D3D12_INPUT_ELEMENT_DESC	layoutEditorObject[2];
};