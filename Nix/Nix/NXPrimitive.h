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
	~NXPrimitive() {}

	friend class NXTriangle;

	virtual void Update();
	virtual void Render();
	virtual void Release();

	// 自动计算顶点的切线数据。
	void CalculateTangents(bool bUpdateVertexIndexBuffer = false);

	void SetMaterialPBR(NXPBRMaterial* mat);

	NXPBRMaterial* GetPBRMaterial() const;
	AABB GetAABBWorld();
	AABB GetAABBLocal() const;
	NXTriangle GetTriangle(int faceIndex);

	virtual void UpdateSurfaceAreaInfo();		// 更新此物体表面积相关的信息。
	virtual float GetSurfaceArea();				// 计算表面积

	ID3D11Buffer* GetMaterialBuffer() const { return m_cbMaterial.Get(); }

	void SetTangibleLight(NXPBRTangibleLight* pTangibleLight) { m_pTangibleLight = pTangibleLight; }
	NXPBRTangibleLight* GetTangibleLight() const { return m_pTangibleLight; }

	virtual bool RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist);

	// 基于表面积对Primitive表面采样
	virtual void SampleForArea(Vector3& o_pos, Vector3& o_norm, float& o_pdfA);
	virtual float GetPdfArea() { return 1.0f / GetSurfaceArea(); }

	// 基于立体角对Primitive表面采样
	virtual void SampleForSolidAngle(const NXHit& hitInfo, Vector3& o_pos, Vector3& o_norm, float& o_pdfW);
	virtual float GetPdfSolidAngle(const NXHit& hitInfo, const Vector3& posLight, const Vector3& normLight, const Vector3& dirLight);

protected:
	virtual void InitVertexIndexBuffer();
	void InitMaterialBuffer();
	void InitAABB();

private:
	NXTriangle SampleTriangle();		// 按面积的PDF采样任一三角形

protected:
	ComPtr<ID3D11Buffer>		m_pVertexBuffer;
	ComPtr<ID3D11Buffer>		m_pIndexBuffer;

	std::vector<VertexPNTT>		m_vertices;
	std::vector<UINT>			m_indices;
	std::vector<Vector3>		m_points;	// vertices position 序列

	ConstantBufferMaterial		m_cbDataMaterial;
	ComPtr<ID3D11Buffer>		m_cbMaterial;
	NXPBRMaterial*				m_pPBRMaterial;

	NXPBRTangibleLight*	m_pTangibleLight;	// 可以将Primitive设置为光源

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
	NXTriangle(NXPrimitive* pShape, int startIndex);
	~NXTriangle() {};

	float Area() const;
	VertexPNTT GetVertex(int VertexId) const;
	bool RayCast(const Ray& localRay, NXHit& outHitInfo, float& outDist);

private:
	int startIndex;
	NXPrimitive* pShape;
};
