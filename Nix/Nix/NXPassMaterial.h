#pragma once
#include "NXPBRMaterial.h"
#include "NXReadbackData.h"

struct NXPassMatLayout
{
	int cbvSpaceNum = 0;
	std::vector<int> cbvSlotNum = {};
	int srvSpaceNum = 0;
	std::vector<int> srvSlotNum = {};

	// ComputePass专用
	int uavSpaceNum = 0;
	std::vector<int> uavSlotNum = {};
};

class NXPassMaterial : public NXMaterial
{
public:
	NXPassMaterial(const std::string& name, const std::filesystem::path& shaderPath = {});

	// 设置当前shader需要几个cbv space
	void RegisterCBVSpaceNum(int spaceNum) { m_layout.cbvSpaceNum = spaceNum; m_layout.cbvSlotNum.resize(spaceNum); }

	// 设置当前cbv需要几个slot（默认space0）
	void RegisterCBVSlotNum(int slotNum, int spaceIndex = 0) { m_layout.cbvSlotNum[spaceIndex] = slotNum; }

	// 设置当前shader需要几个srv space
	void RegisterSRVSpaceNum(int spaceNum) { m_layout.srvSpaceNum = spaceNum; m_layout.srvSlotNum.resize(spaceNum); }

	// 设置当前srv需要几个slot（默认space0）
	void RegisterSRVSlotNum(int slotNum, int spaceIndex = 0) { m_layout.srvSlotNum[spaceIndex] = slotNum; }

	// 设置当前shader需要几个uav space
	void RegisterUAVSpaceNum(int spaceNum) { m_layout.uavSpaceNum = spaceNum; m_layout.uavSlotNum.resize(spaceNum); }

	// 设置当前uav需要几个slot（默认space0）
	void RegisterUAVSlotNum(int slotNum, int spaceIndex = 0) { m_layout.uavSlotNum[spaceIndex] = slotNum; }

	virtual void RegisterRTVNum(const std::vector<DXGI_FORMAT>& rtFormats) {}
	virtual void RegisterDSV(DXGI_FORMAT dsvFormat) {}

	virtual void FinalizeLayout() {}

	NXShadingModel GetShadingModel() override { return NXShadingModel::Unlit; }
	void SetEntryNameVS(const std::wstring& entryName) { m_entryNameVS = entryName; }
	void SetEntryNamePS(const std::wstring& entryName) { m_entryNamePS = entryName; }
	void SetEntryNameCS(const std::wstring& entryName) { m_entryNameCS = entryName; }

	void SetShaderFilePath(const std::filesystem::path& shaderFilePath);
	void SetConstantBuffer(int spaceIndex, int slotIndex, const NXConstantBufferImpl* pCBuffer);

	void AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& staticSampler);
	void AddStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addrUVW);

	virtual void Compile() = 0;
	virtual void Update() = 0;
	virtual void Render(ID3D12GraphicsCommandList*) = 0;
	virtual void Release() = 0;

	void InitRootParams();

protected:
	// 当前 Nix 的根参数（和采样器）-寄存器的布局规则：
	// 根参数索引(rootParameterIndex)按以下顺序排列：
	// 1. CBV：每个slot,space占用一个根参数
	// 	  按：space0的b0，b1，...，space1的b0，b1，...的顺序排列根参数
	// 2. SRV：
	//	  每个space占用一个描述符Table，并占用一个根参数。
	// 3. UAV：
	//	  每个space占用一个描述符Table，并占用一个根参数。
	// 4. 任何情况下都不使用根常量
	// 5. 采样器始终使用StaticSampler，暂不考虑space和动态Sampler的问题，目前够用了
	std::vector<D3D12_ROOT_PARAMETER> m_rootParams;
	std::vector<D3D12_DESCRIPTOR_RANGE> m_srvRanges; // 这俩range必须用成员变量保存
	std::vector<D3D12_DESCRIPTOR_RANGE> m_uavRanges;
    NXPassMatLayout m_layout;
	
	std::filesystem::path m_shaderFilePath;
	std::wstring m_entryNameVS;
	std::wstring m_entryNamePS;
	std::wstring m_entryNameCS;
	
	std::vector<D3D12_STATIC_SAMPLER_DESC> m_staticSamplers;
    std::vector<std::vector<const NXConstantBufferImpl*>> m_cbuffers;
};

class NXGraphicPassMaterial : public NXPassMaterial
{
public:
	NXGraphicPassMaterial(const std::string& name, const std::filesystem::path& shaderPath = {});

	// 设置当前shader的RT数量和格式
	void RegisterRTVNum(const std::vector<DXGI_FORMAT>& rtFormats) override { m_rtFormats = rtFormats; }

	// 设置当前shader的DS格式
	void RegisterDSV(DXGI_FORMAT dsvFormat) override { m_dsvFormat = dsvFormat; }
	void FinalizeLayout() override;

	void SetInputTex(int spaceIndex, int slotIndex, const Ntr<NXResource>& pTex);
	void SetOutputRT(int index, const Ntr<NXResource>& pRT);
	void SetOutputDS(const Ntr<NXResource>& pDS);

	void SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& desc);
	void SetBlendState(const D3D12_BLEND_DESC& desc);
	void SetRasterizerState(const D3D12_RASTERIZER_DESC& desc);
	void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& desc);
	void SetSampleDescAndMask(UINT Count, UINT Quality, UINT Mask);
	void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);
	void SetStencilRef(const UINT stencilRef) { m_stencilRef = stencilRef; }
	void SetRenderTargetMesh(const std::string& rtSubMeshName) { m_rtSubMeshName = rtSubMeshName; }
	const std::string& GetRenderTargetMesh() const { return m_rtSubMeshName; }

	void Compile() override;
	void Update() override {}
	void Render(ID3D12GraphicsCommandList* pCmdList) override;
	void Release() override {}

	void RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList);
	void RenderBefore(ID3D12GraphicsCommandList* pCmdList);

private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_psoDesc;

	std::vector<std::vector<Ntr<NXResource>>> m_pInTexs; // [space][slot]
	std::vector<Ntr<NXResource>> m_pOutRTs;
	Ntr<NXResource> m_pOutDS;
	std::vector<DXGI_FORMAT> m_rtFormats;
	DXGI_FORMAT m_dsvFormat;
	UINT m_stencilRef;
	std::string m_rtSubMeshName;
};

// 如果是UAV类型做pass output，用法比较多，所以单独封装一个结构体
struct NXResourceUAV
{
	// 资源本体
	Ntr<NXResource> pRes;

	// 如果是Buffer类型的，底层会同时创建buffer本体和计数器两个资源；如果用计数器，此处设为true
	bool useBufferUAVCounter = false; 

	// 如果是Texture类型的，可能需要给每个mip等级封装专门UAV视图，此处指定mip等级。
	int texMipSlice = -1; // -1表示不指定，使用默认视图 
	
	NXResourceUAV() = default;
	NXResourceUAV(const Ntr<NXResource>& res, bool useCounter = false) : pRes(res), useBufferUAVCounter(useCounter) {}
	NXResourceUAV(const Ntr<NXResource>& res, int mipSlice) : pRes(res), texMipSlice(mipSlice) {}
};

class NXComputePassMaterial : public NXPassMaterial
{
public:
	NXComputePassMaterial(const std::string& name, const std::filesystem::path& shaderPath = {});

	void FinalizeLayout() override;

	void SetInput(int spaceIndex, int slotIndex, const Ntr<NXResource>& pRes);
	void SetOutput(int spaceIndex, int slotIndex, const Ntr<NXResource>& pRes, bool isUAVCounter = false);
	void SetOutput(int spaceIndex, int slotIndex, const Ntr<NXResource>& pRes, int mipSlice);

	void Compile() override;
	void Update() override {}
	void Render(ID3D12GraphicsCommandList* pCmdList) override;
	void Release() override {}

	void RenderSetTargetAndState(ID3D12GraphicsCommandList* pCmdList);
	void RenderBefore(ID3D12GraphicsCommandList* pCmdList);

private:
	D3D12_COMPUTE_PIPELINE_STATE_DESC m_csoDesc;
	Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_pCommandSig;
	std::vector<std::vector<Ntr<NXResource>>> m_pInRes;
	std::vector<std::vector<NXResourceUAV>> m_pOutRes;
};

class NXReadbackPassMaterial : public NXPassMaterial
{
public:
	NXReadbackPassMaterial(const std::string& name) : NXPassMaterial(name) {}

	void SetInput(Ntr<NXResource> pRes) { m_pReadbackBuffer = pRes; }
	void SetOutput(Ntr<NXReadbackData>& pOutData) { m_pOutData = pOutData; }

	void Compile() override {}
	void Update() override {}
	void Render(ID3D12GraphicsCommandList* pCmdList) override;
	void Release() override {}

private:
	Ntr<NXResource> m_pReadbackBuffer; // input
	Ntr<NXReadbackData> m_pOutData;		// output
};
