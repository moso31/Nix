#pragma once
#include <filesystem>
#include "BaseDefs/DX12.h"
#include "CommittedAllocator.h"
#include "NXTransform.h"
#include "DirectXTex.h"
#include "ShaderStructures.h"
#include "Ntr.h"

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

class NXScene;
class NXTextureCube;
class NXCubeMap : public NXTransform
{
public:
	NXCubeMap(NXScene* pScene);
	~NXCubeMap() {}

	// 1. 顶点初始化
	// 2. 构建相机
	// 3. 如果是dds，加载；如果是hdr，强转成dds，加载
	//		LoadDDS：创建一个NXTextureCube+SRV
	// 4. 生成IrradMap
	//		如果是SH方法，最终生成的结果保存为cbData
	//		如果是Tex方法，
	// 5. 生成PreFilterMap
	//		GeneratePreFilterMap()：
	//		1. 创建一个CubeMap + SRV，同时准备shader和渲染管线相关的内容
	//		2. for 循环 渲染30个RTV，并依次绘制
	bool Init(const std::filesystem::path& filePath);
	void Update() override;
	void UpdateViewParams();
	void Render();
	void Release() override;

	Ntr<NXTextureCube> GenerateCubeMap(Ntr<NXTexture2D>& pTexHDR);
	void GenerateIrradianceSHFromHDRI(Ntr<NXTexture2D>& pTexHDR);
	void GenerateIrradianceSHFromCubeMap();

	void GenerateIrradianceMap();
	void GeneratePreFilterMap();

	size_t GetSRVCubeMap();
	size_t GetSRVCubeMapPreview2D();
	size_t GetSRVIrradianceMap();
	size_t GetSRVPreFilterMap();

	ID3D11Buffer* GetConstantBufferParams() { return m_cb.Get(); }

	void SetIntensity(float val) { m_cbData.intensity = val; }
	float* GetIntensity() { return &m_cbData.intensity; }

	void SetIrradMode(int val) { m_cbData.irradMode = Vector4((float)val); };

	void SaveHDRAsDDS(Ntr<NXTextureCube>& pTexture, const std::filesystem::path& filePath);
	void LoadDDS(const std::filesystem::path& filePath);

private:
	void InitVertex();
	void UpdateVBIB();

private:
	NXScene* m_pScene;

	Matrix m_mxCubeMapProj;
	Matrix m_mxCubeMapView[6];

	std::vector<VertexP>		m_vertices;
	std::vector<UINT>			m_indices;
	ComPtr<ID3D11Buffer>		m_pVertexBuffer;
	ComPtr<ID3D11Buffer>		m_pIndexBuffer;

	std::vector<VertexP>		m_verticesCubeBox;
	std::vector<UINT>			m_indicesCubeBox;
	ComPtr<ID3D11Buffer>		m_pVertexBufferCubeBox;
	ComPtr<ID3D11Buffer>		m_pIndexBufferCubeBox;

	Ntr<NXTextureCube>			m_pTexCubeMap;
	Ntr<NXTextureCube>			m_pTexIrradianceMap;
	Ntr<NXTextureCube>			m_pTexPreFilterMap;

	Vector3 m_shIrradianceMap[9];
	Vector3 m_shIrradianceMap_CPU[9];

	// 生成使用独立的 allocator 来管理 CubeMap 的 cb
	CommittedAllocator* m_cbAllocator;
	CommittedResourceData<ConstantBufferCubeMap> m_cbData;

	////////////////////////////////////////////////////////////////////////////
	//// Deprecated functions...
	////////////////////////////////////////////////////////////////////////////


public:
	void GenerateIrradianceSHFromHDRI_Deprecated(NXTexture2D* pTexHDR);
	size_t GetSRVIrradianceSH() { return m_pSRVIrradianceSH; }

private:
	size_t	m_pSRVIrradianceSH;
};