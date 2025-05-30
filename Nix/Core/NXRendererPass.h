#pragma once
#include "BaseDefs/DX12.h"
#include <filesystem>
#include "Ntr.h"
#include "NXCommonTexDefinition.h"
#include "NXTexture.h"

class NXPassTexture
{
public:
	NXPassTexture(Ntr<NXTexture> pTexture, uint32_t passSlotIndex = -1) :
		pTexture(pTexture), slotIndex(passSlotIndex) {}

	Ntr<NXTexture> pTexture;
	uint32_t slotIndex = -1;
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

class NXRendererPass
{
public:
	NXRendererPass();
	virtual ~NXRendererPass() {}

	virtual void SetupInternal() = 0;

	void SetPassName(const std::string& passName) { m_passName = passName; }

	void SetInputTex(NXCommonTexEnum eCommonTex, uint32_t slotIndex);
	void SetInputTex(const Ntr<NXTexture>& pTex, uint32_t slotIndex);
	void SetOutputRT(const Ntr<NXTexture>& pTex, uint32_t rtIndex);
	void SetOutputDS(const Ntr<NXTexture>& pTex);

	Ntr<NXTexture> GetInputTex(uint32_t slotIndex) { return m_pInTexs[slotIndex]; }
	Ntr<NXTexture> GetOutputRT(uint32_t index) { return m_pOutRTs[index]; }
	Ntr<NXTexture> GetOutputDS() { return m_pOutDS; }

	void SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& desc);
	void SetRenderTargetMesh(const std::string& rtSubMeshName);
	void SetBlendState(const D3D12_BLEND_DESC& desc);
	void SetRasterizerState(const D3D12_RASTERIZER_DESC& desc);
	void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& desc);
	void SetSampleDescAndMask(UINT Count, UINT Quality, UINT Mask);
	void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);
	void SetShaderFilePath(const std::filesystem::path& shaderFilePath);

	void SetStencilRef(const UINT stencilRef) { m_stencilRef = stencilRef; }

	// 2024.5.26 ��ǰ Nix �ĸ��������Ͳ�������-�Ĵ����Ĳ��ֹ���
	// 1. ÿ��CBVռ��һ��������
	// 2. ������SRV������ЩSRV�����ŵ�һ��������Table��Ҹ�Table��ʼ��ռ�����һ����������
	// ��2+. UAV Ŀǰ��ʱ��û�漰�����õ�����˵��
	// 3. �κ�����¶���ʹ�ø�����
	// 4. ������ʼ��ʹ��StaticSampler�������Ƕ�̬Sampler��Ŀǰ������
	void SetRootParams(int CBVNum, int SRVUAVNum);

	// ���þ�̬CBV��
	// ע�⣺��һ��Pass��������óɾ�̬CBV����ô���CBV��ӳ���ַ����Pass�����������ڲ���ı䡣
	// Ӧ�ø���Passʵ����������Ƿ����ã�������äĿȫ�����á�
	// rootParamIndex: ������������
	// slotIndex: ���������������������ṩ�����rootParamIndex��ͬ��
	// gpuVirtAddr: CBV��gpu�����ַ
	void SetStaticRootParamCBV(int rootParamIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs);
	void SetStaticRootParamCBV(int rootParamIndex, int slotIndex, const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>* gpuVirtAddrs);

	void AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& staticSampler);
	void AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW);

	// ����PSO
	// ����������������Ϻ��ٵ����������������PSO
	void InitPSO();

	virtual void RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList);
	virtual void RenderBefore(ID3D12GraphicsCommandList* pCmdList);
	virtual void Render(ID3D12GraphicsCommandList* pCmdList);

	void Release() {}

private:
	std::string	m_passName;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC		m_psoDesc;
	ComPtr<ID3D12PipelineState>				m_pPSO;
	ComPtr<ID3D12RootSignature>				m_pRootSig;
	UINT m_stencilRef;
	Vector2 m_viewPortSize;

	std::vector<Ntr<NXTexture>>				m_pInTexs;
	std::vector<Ntr<NXTexture>>				m_pOutRTs;
	Ntr<NXTexture>							m_pOutDS;

	std::filesystem::path					m_shaderFilePath;

	// pass ʹ�õĸ�����
	std::vector<D3D12_ROOT_PARAMETER>		m_rootParams;

	// pass ʹ�õ� srv/uav ��������
	std::vector<D3D12_DESCRIPTOR_RANGE>		m_srvUavRanges;

	// pass ʹ�õľ�̬������
	std::vector<D3D12_STATIC_SAMPLER_DESC>	m_staticSamplers;

	// Pass������Ҫ�������������Pass������ЩCB��
	std::vector<NXCBVManagement>			m_cbvManagements;

	// rt ʹ�õ� subMesh �����ơ�
	// ʵ����Ⱦʱ�����������ȷ��ʹ�� NXSubMeshGeometryEditor ���ĸ� subMesh ��Ϊ RT.
	// һ�㲻��Ҫ���ã�����Ĭ�ϵ� "_RenderTarget" ���У���Ҳ�����������
	// ������Ⱦ CubeMap �� Pass����Ҫ����Ϊ "_CubeMapSphere"��ʹ��һ��Բ�ε� RT��
	std::string								m_rtSubMeshName;
};
