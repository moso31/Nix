#pragma once
#include <filesystem>
#include "BaseDefs/DX12.h"
#include "BaseDefs/NixCore.h"
#include "NXTransform.h"
#include "DirectXTex.h"
#include "ShaderStructures.h"
#include "Ntr.h"
#include "NXConstantBuffer.h"

#define NXCUBEMAP_FACE_COUNT 6
#define NXROUGHNESS_FILTER_COUNT 5

struct_cbuffer ConstantBufferImageData
{
	Vector4 currImgSize; // xy: size zw: sizeInv
	Vector4 nextImgSize; // xy: size zw: sizeInv
};

struct_cbuffer ConstantBufferIrradSH
{
	ConstantBufferIrradSH() {}
	Vector4 irradSH[9];
};

struct_cbuffer ConstantBufferCubeMap
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

struct_cbuffer ConstantBufferBaseWVP
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
	virtual ~NXCubeMap() {}

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
	void Release() override;

	using GenerateCubeMapCallback = std::function<void()>;
	void GenerateCubeMap(Ntr<NXTexture2D>& pTexHDR, GenerateCubeMapCallback pCubeMapCallBack);
	void GenerateIrradianceSHFromHDRI(Ntr<NXTexture2D>& pTexHDR);
	void GenerateIrradianceSHFromCubeMap();

	void GenerateIrradianceMap();
	void GeneratePreFilterMap();

	void WaitForCubeMapTexsReady();

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCubeMap();
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCubeMapPreview2D();
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVIrradianceMap();
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVPreFilterMap();

	Ntr<NXTexture2D> GetCubeMap();
	Ntr<NXTexture2D> GetIrradianceMap();
	Ntr<NXTexture2D> GetPreFilterMap();

	const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>& GetCBObjectParams() { return m_cbCubeWVPMatrix.GetFrameGPUAddresses(); }
	const MultiFrame<D3D12_GPU_VIRTUAL_ADDRESS>& GetCBDataParams() { return m_cbCubeMap.GetFrameGPUAddresses(); }

	void SetIntensity(float val);
	float GetIntensity() { return m_cbDataCubeMap.intensity; }

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

	ConstantBufferCubeMap m_cbDataCubeMap;
	NXConstantBuffer<ConstantBufferCubeMap> m_cbCubeMap;
	ConstantBufferBaseWVP m_cbDataCubeWVPMatrix;
	NXConstantBuffer<ConstantBufferBaseWVP> m_cbCubeWVPMatrix;

	size_t	m_pSRVIrradianceSH;

	float m_prefilterMapSize;
	float m_cubeMapSize;

	ComPtr<ID3D12CommandQueue> m_pCommandQueue;
	ComPtr<ID3D12Fence> m_pFence;
	uint64_t m_nFenceValue;

	ComPtr<ID3D12RootSignature> m_pRootSigCubeMap;
	ComPtr<ID3D12PipelineState> m_pPSOCubeMap;

	ComPtr<ID3D12RootSignature> m_pRootSigPreFilterMap;
	ComPtr<ID3D12PipelineState> m_pPSOPreFilterMap;

	std::promise<void> m_promiseCubeMapTexsReady;
	std::future<void> m_futureCubeMapTexsReady;

public:
	void GenerateIrradianceSHFromHDRI_Deprecated(NXTexture2D* pTexHDR);
	size_t GetSRVIrradianceSH() { return m_pSRVIrradianceSH; }
};