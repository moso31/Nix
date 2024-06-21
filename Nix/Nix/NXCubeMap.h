#pragma once
#include <filesystem>
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXTransform.h"
#include "DirectXTex.h"
#include "ShaderStructures.h"
#include "Ntr.h"
#include "NXBuffer.h"

#define NXCUBEMAP_FACE_COUNT 6
#define NXROUGHNESS_FILTER_COUNT 5

struct ConstantBufferImageData
{
	Vector4 currImgSize; // xy: size zw: sizeInv
	Vector4 nextImgSize; // xy: size zw: sizeInv
};

struct ConstantBufferIrradSH
{
	ConstantBufferIrradSH() {}
	Vector4 irradSH[9];
};

struct ConstantBufferCubeMap
{
	ConstantBufferCubeMap() : intensity(1.0f), irradMode(0.0f) {}
	Vector4 irradSH0123x;
	Vector4 irradSH4567x;
	Vector4 irradSH0123y;
	Vector4 irradSH4567y;
	Vector4 irradSH0123z;
	Vector4 irradSH4567z;
	Vector3 irradSH8xyz;
	float intensity;
	Vector4 irradMode;
};

struct ConstantBufferBaseWVP
{
	Matrix world;
	Matrix view;
	Matrix projection;
};

class NXScene;
class NXTextureCube;
class NXCubeMap : public NXTransform
{
public:
	NXCubeMap(NXScene* pScene);
	~NXCubeMap() {}

	// 1. �����ʼ��
	// 2. �������
	// 3. �����dds�����أ������hdr��ǿת��dds������
	//		LoadDDS������һ��NXTextureCube+SRV
	// 4. ����IrradMap
	//		�����SH�������������ɵĽ������ΪcbData
	//		�����Tex������
	// 5. ����PreFilterMap
	//		GeneratePreFilterMap()��
	//		1. ����һ��CubeMap + SRV��ͬʱ׼��shader����Ⱦ������ص�����
	//		2. for ѭ�� ��Ⱦ30��RTV�������λ���
	bool Init(const std::filesystem::path& filePath);
	void Update() override;
	void UpdateViewParams();
	void Render(ID3D12GraphicsCommandList* pCmdList);
	void Release() override;

	Ntr<NXTextureCube> GenerateCubeMap(Ntr<NXTexture2D>& pTexHDR);
	void GenerateIrradianceSHFromHDRI(Ntr<NXTexture2D>& pTexHDR);
	void GenerateIrradianceSHFromCubeMap();

	void GenerateIrradianceMap();
	void GeneratePreFilterMap();

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCubeMap();
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCubeMapPreview2D();
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVIrradianceMap();
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVPreFilterMap();

	Ntr<NXTexture2D> GetCubeMap() { return m_pTexCubeMap; }
	Ntr<NXTexture2D> GetIrradianceMap() { return m_pTexIrradianceMap; }
	Ntr<NXTexture2D> GetPreFilterMap() { return m_pTexPreFilterMap; }

	const D3D12_GPU_VIRTUAL_ADDRESS& GetCBObjectParams() { return m_cbObject.GetGPUHandle(); }
	const D3D12_GPU_VIRTUAL_ADDRESS& GetCBDataParams() { return m_cbData.GetGPUHandle(); }

	void SetIntensity(float val);
	float* GetIntensity() { return &m_cbData.Current().intensity; }

	void SetIrradMode(int val);

	void SaveHDRAsDDS(Ntr<NXTextureCube>& pTexture, const std::filesystem::path& filePath);
	void LoadDDS(const std::filesystem::path& filePath);

private:
	void InitVertex();
	void InitRootSignature();

private:
	NXScene* m_pScene;

	Matrix m_mxCubeMapProj;
	Matrix m_mxCubeMapView[6];

	std::vector<VertexP>		m_vertices;
	std::vector<UINT>			m_indices;

	std::vector<VertexP>		m_verticesCubeBox;
	std::vector<UINT>			m_indicesCubeBox;

	Ntr<NXTextureCube>			m_pTexCubeMap;
	Ntr<NXTextureCube>			m_pTexIrradianceMap;
	Ntr<NXTextureCube>			m_pTexPreFilterMap;

	Vector3 m_shIrradianceMap[9];
	Vector3 m_shIrradianceMap_CPU[9];

	// ����ʹ�ö����� allocator ������ CubeMap �� cb
	CommittedAllocator* m_cbAllocator;
	NXBuffer<ConstantBufferBaseWVP> m_cbObject[6];  // [6] for 6 faces����ȻNXBuffer<>::CreateBufferҲ֧��һ���Դ���6������
	NXBuffer<ConstantBufferCubeMap> m_cbData;

	size_t	m_pSRVIrradianceSH;

	ComPtr<ID3D12CommandQueue> m_pCommandQueue;

	ComPtr<ID3D12RootSignature> m_pRootSigCubeMap;
	ComPtr<ID3D12PipelineState> m_pPSOCubeMap;

	ComPtr<ID3D12RootSignature> m_pRootSigPreFilterMap;
	ComPtr<ID3D12PipelineState> m_pPSOPreFilterMap;

	ComPtr<ID3D12Fence> m_pFence;
	UINT64 m_fenceValue;

public:
	void GenerateIrradianceSHFromHDRI_Deprecated(NXTexture2D* pTexHDR);
	size_t GetSRVIrradianceSH() { return m_pSRVIrradianceSH; }
};