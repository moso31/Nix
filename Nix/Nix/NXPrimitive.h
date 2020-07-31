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

	void SetMaterial(const shared_ptr<NXMaterial>& material);
	void SetMaterialPBR(const shared_ptr<NXPBRMaterial>& mat);

	shared_ptr<NXPBRMaterial> GetPBRMaterial() const;
	AABB GetAABBWorld();
	AABB GetAABBLocal() const;
	NXTriangle GetTriangle(int faceIndex);

	// ��ʵ���ǻ�ȡ�����ʽ��thisָ�롣
	// ����ʹ����shared_ptr��ÿ�λ�ȡ�����ʽ��this����Ҫִ��һ�ζ�̬ת�����ܳ����ܡ�
	// Ϊÿһ��Primitive��һ��ָ��洢this���ڵ���PathIntegrator�����п��Խ�ʡ���4%-8%���ҵ�ȫ�����ܿ������ǳ����㡣
	shared_ptr<NXPrimitive> GetSelf();

	void UpdateSurfaceAreaInfo();		// ���´�����������ص���Ϣ��
	float GetSurfaceArea();				// ��������
	NXTriangle SampleTriangle();		// �������PDF������һ������

	ID3D11ShaderResourceView* GetTextureSRV() const { return m_pTextureSRV; }
	ID3D11Buffer* GetMaterialBuffer() const { return m_cbMaterial; }

	void SetTangibleLight(shared_ptr<NXPBRTangibleLight> pTangibleLight) { m_pTangibleLight = pTangibleLight; }
	shared_ptr<NXPBRTangibleLight> GetTangibleLight() const { return m_pTangibleLight; }

	virtual bool RayCast(const Ray& localRay, NXHit& outHitInfo, float& outDist);

	// ���ڱ������Primitive�������
	virtual void SampleForArea(Vector3& o_pos, Vector3& o_norm, float& o_pdfA);
	float GetPdf(const NXHit& hitInfo, const Vector3& direction) { return 1.0f / GetSurfaceArea(); }

	// ��������Ƕ�Primitive�������
	virtual void SampleForSolidAngle(const Vector3& point, Vector3& o_pos, Vector3& o_norm, float& o_pdfW);

protected:
	void InitVertexIndexBuffer();
	void InitAABB();

protected:
	ID3D11Buffer*				m_pVertexBuffer;
	ID3D11Buffer*				m_pIndexBuffer;
	ID3D11ShaderResourceView*	m_pTextureSRV;

	vector<VertexPNT>			m_vertices;
	vector<USHORT>				m_indices;
	vector<Vector3>				m_points;	// vertices position ����

	ConstantBufferMaterial		m_cbDataMaterial;
	ID3D11Buffer*				m_cbMaterial;
	shared_ptr<NXMaterial>		m_pMaterial;
	shared_ptr<NXPBRMaterial>	m_pPBRMaterial;

	shared_ptr<NXPBRTangibleLight>	m_pTangibleLight;	// ���Խ�Primitive����Ϊ��Դ
	shared_ptr<NXPrimitive>		m_pThis;

	AABB m_aabb;
	float m_fArea;		// ��¼��ǰMesh���ܱ������
	vector<float> m_triangleAreas;	// ��¼��ÿ�������εı�������ۻ�CDF�����Ȳ���ʱ�����á�

private:
	bool m_bEnableNormal;			// �Ƿ�ʹ�������Դ��ķ�������
	bool m_bEnableTangent;			// �Ƿ�ʹ�������Դ�������������
	bool m_bEnableNormalDerivative;	// �Ƿ����dndu��dndv
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
