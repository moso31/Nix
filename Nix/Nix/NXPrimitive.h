#pragma once
#include "ShaderStructures.h"
#include "NXTransform.h"

class NXBox;
class NXSphere;
class NXCylinder;
class NXCone;
class NXPlane;
class NXTriangle;

class NXHit;
class NXPBRMaterial;

class NXPrimitive : public NXTransform
{
public:
	NXPrimitive();
	virtual ~NXPrimitive() {}

	friend class NXTriangle;

	virtual void Update();
	virtual void Render();
	virtual void Release();

	void SetMaterial(const shared_ptr<NXMaterial>& material);
	void SetMaterialPBR(const shared_ptr<NXPBRMaterial>& mat);

	shared_ptr<NXPBRMaterial> GetPBRMaterial() const;
	AABB GetAABBWorld();
	AABB GetAABBLocal() const;
	NXTriangle GetTriangle(int faceIndex);

	ID3D11ShaderResourceView* GetTextureSRV() const { return m_pTextureSRV; }
	ID3D11Buffer* GetMaterialBuffer() const { return m_cbMaterial; }

	//VertexPNT GetVertexPNT() { return }

	virtual bool RayCast(const Ray& localRay, NXHit& outHitInfo, float& outDist);

	// 在当前Primitive表面上进行采样。随机挑选表面上任意一点。
	virtual void SampleFromSurface(int faceIndex, Vector3& out_hitPos, Vector3& out_hitNorm, float& out_pdf);

protected:
	void InitVertexIndexBuffer();
	void InitAABB();

protected:
	ID3D11Buffer*				m_pVertexBuffer;
	ID3D11Buffer*				m_pIndexBuffer;
	ID3D11ShaderResourceView*	m_pTextureSRV;

	vector<VertexPNT>			m_vertices;
	vector<USHORT>				m_indices;
	vector<Vector3>				m_points;	// vertices position 序列

	ConstantBufferMaterial		m_cbDataMaterial;
	ID3D11Buffer*				m_cbMaterial;
	shared_ptr<NXMaterial>		m_pMaterial;
	shared_ptr<NXPBRMaterial>	m_pPBRMaterial;

	AABB m_aabb;

private:
	bool m_bEnableNormal;			// 是否使用纹理自带的法线数据
	bool m_bEnableTangent;			// 是否使用纹理自带的切向量数据
	bool m_bEnableNormalDerivative;	// 是否计算dndu，dndv
};

class NXTriangle
{
public:
	NXTriangle(const shared_ptr<NXPrimitive>& pShape, int startIndex);
	~NXTriangle() {};

	float Area() const;
	VertexPNT GetPointData(int PointId) const;
	bool RayCast(const Ray& localRay, NXHit& outHitInfo, float& outDist);

private:
	int startIndex;
	shared_ptr<NXPrimitive> pShape;
};
