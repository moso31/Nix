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

	// �Զ����㶥����������ݡ�
	void CalculateTangents(bool bUpdateVertexIndexBuffer = false);

	void SetMaterialPBR(NXPBRMaterial* mat);

	NXPBRMaterial* GetPBRMaterial() const;
	AABB GetAABBWorld();
	AABB GetAABBLocal() const;
	NXTriangle GetTriangle(int faceIndex);

	virtual void UpdateSurfaceAreaInfo();		// ���´�����������ص���Ϣ��
	virtual float GetSurfaceArea();				// ��������

	ID3D11Buffer* GetMaterialBuffer() const { return m_cbMaterial.Get(); }

	void SetTangibleLight(NXPBRTangibleLight* pTangibleLight) { m_pTangibleLight = pTangibleLight; }
	NXPBRTangibleLight* GetTangibleLight() const { return m_pTangibleLight; }

	virtual bool RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist);

	// ���ڱ������Primitive�������
	virtual void SampleForArea(Vector3& o_pos, Vector3& o_norm, float& o_pdfA);
	virtual float GetPdfArea() { return 1.0f / GetSurfaceArea(); }

	// ��������Ƕ�Primitive�������
	virtual void SampleForSolidAngle(const NXHit& hitInfo, Vector3& o_pos, Vector3& o_norm, float& o_pdfW);
	virtual float GetPdfSolidAngle(const NXHit& hitInfo, const Vector3& posLight, const Vector3& normLight, const Vector3& dirLight);

protected:
	virtual void InitVertexIndexBuffer();
	void InitMaterialBuffer();
	void InitAABB();

private:
	NXTriangle SampleTriangle();		// �������PDF������һ������

protected:
	ComPtr<ID3D11Buffer>		m_pVertexBuffer;
	ComPtr<ID3D11Buffer>		m_pIndexBuffer;

	std::vector<VertexPNTT>		m_vertices;
	std::vector<UINT>			m_indices;
	std::vector<Vector3>		m_points;	// vertices position ����

	ConstantBufferMaterial		m_cbDataMaterial;
	ComPtr<ID3D11Buffer>		m_cbMaterial;
	NXPBRMaterial*				m_pPBRMaterial;

	NXPBRTangibleLight*	m_pTangibleLight;	// ���Խ�Primitive����Ϊ��Դ

	AABB m_aabb;
	float m_fArea;		// ��¼��ǰMesh���ܱ������
	std::vector<float> m_triangleAreas;	// ��¼��ÿ�������εı�������ۻ�CDF�����Ȳ���ʱ�����á�

private:
	bool m_bEnableNormal;			// �Ƿ�ʹ�������Դ��ķ�������
	bool m_bEnableTangent;			// �Ƿ�ʹ�������Դ�������������
	bool m_bEnableNormalDerivative;	// �Ƿ����dndu��dndv
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
