#pragma once
#include "ShaderStructures.h"
#include "NXTransform.h"

class NXHit;
class NXPBRMaterial;
class NXPBRTangibleLight;

class NXPrimitive : public NXTransform
{
public:
	NXPrimitive();
	virtual ~NXPrimitive() {}

	friend class NXTriangle;

	virtual void Update();
	virtual void Render();
	virtual void Release();

	void SetMaterialPBR(const std::shared_ptr<NXPBRMaterial>& mat);

	std::shared_ptr<NXPBRMaterial> GetPBRMaterial() const;
	AABB GetAABBWorld();
	AABB GetAABBLocal() const;
	NXTriangle GetTriangle(int faceIndex);

	// 其实就是获取子类格式的this指针。
	// 由于使用了std::shared_ptr，每次获取子类格式的this都需要执行一次动态转换，很吃性能。
	// 为每一个Primitive开一个指针存储this，在单次PathIntegrator计算中可以节省大概4%-8%左右的全局性能开销。非常划算。
	std::shared_ptr<NXPrimitive> GetSelf();

	virtual void UpdateSurfaceAreaInfo();		// 更新此物体表面积相关的信息。
	virtual float GetSurfaceArea();				// 计算表面积

	ID3D11ShaderResourceView* GetTextureSRV() const { return m_pTextureSRV; }
	ID3D11Buffer* GetMaterialBuffer() const { return m_cbMaterial; }

	void SetTangibleLight(std::shared_ptr<NXPBRTangibleLight> pTangibleLight) { m_pTangibleLight = pTangibleLight; }
	std::shared_ptr<NXPBRTangibleLight> GetTangibleLight() const { return m_pTangibleLight; }

	virtual bool RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist);

	// 基于表面积对Primitive表面采样
	virtual void SampleForArea(Vector3& o_pos, Vector3& o_norm, float& o_pdfA);
	virtual float GetPdfArea() { return 1.0f / GetSurfaceArea(); }

	// 基于立体角对Primitive表面采样
	virtual void SampleForSolidAngle(const NXHit& hitInfo, Vector3& o_pos, Vector3& o_norm, float& o_pdfW);
	virtual float GetPdfSolidAngle(const NXHit& hitInfo, const Vector3& posLight, const Vector3& normLight, const Vector3& dirLight);

protected:
	void InitVertexIndexBuffer();
	void InitAABB();

private:
	NXTriangle SampleTriangle();		// 按面积的PDF采样任一三角形

protected:
	ID3D11Buffer*						m_pVertexBuffer;
	ID3D11Buffer*						m_pIndexBuffer;
	//ID3D11Texture2D*					m_pTexture;
	ID3D11ShaderResourceView*			m_pTextureSRV;

	std::vector<VertexPNT>				m_vertices;
	std::vector<USHORT>					m_indices;
	std::vector<Vector3>				m_points;	// vertices position 序列

	ConstantBufferMaterial				m_cbDataMaterial;
	ID3D11Buffer*						m_cbMaterial;
	std::shared_ptr<NXPBRMaterial>		m_pPBRMaterial;

	std::shared_ptr<NXPBRTangibleLight>	m_pTangibleLight;	// 可以将Primitive设置为光源
	std::shared_ptr<NXPrimitive>		m_pThis;

	AABB m_aabb;
	float m_fArea;		// 纪录当前Mesh的总表面积。
	std::vector<float> m_triangleAreas;	// 纪录由每个三角形的表面积的累积CDF。均匀采样时很有用。

private:
	bool m_bEnableNormal;			// 是否使用纹理自带的法线数据
	bool m_bEnableTangent;			// 是否使用纹理自带的切向量数据
	bool m_bEnableNormalDerivative;	// 是否计算dndu，dndv
};

class NXTriangle
{
public:
	NXTriangle(const std::shared_ptr<NXPrimitive>& pShape, int startIndex);
	~NXTriangle() {};

	float Area() const;
	VertexPNT GetPointData(int PointId) const;
	bool RayCast(const Ray& localRay, NXHit& outHitInfo, float& outDist);

private:
	int startIndex;
	std::shared_ptr<NXPrimitive> pShape;
};
