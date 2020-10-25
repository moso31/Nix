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

	// ��ʵ���ǻ�ȡ�����ʽ��thisָ�롣
	// ����ʹ����std::shared_ptr��ÿ�λ�ȡ�����ʽ��this����Ҫִ��һ�ζ�̬ת�����ܳ����ܡ�
	// Ϊÿһ��Primitive��һ��ָ��洢this���ڵ���PathIntegrator�����п��Խ�ʡ���4%-8%���ҵ�ȫ�����ܿ������ǳ����㡣
	std::shared_ptr<NXPrimitive> GetSelf();

	virtual void UpdateSurfaceAreaInfo();		// ���´�����������ص���Ϣ��
	virtual float GetSurfaceArea();				// ��������

	ID3D11ShaderResourceView* GetTextureSRV() const { return m_pTextureSRV; }
	ID3D11Buffer* GetMaterialBuffer() const { return m_cbMaterial; }

	void SetTangibleLight(std::shared_ptr<NXPBRTangibleLight> pTangibleLight) { m_pTangibleLight = pTangibleLight; }
	std::shared_ptr<NXPBRTangibleLight> GetTangibleLight() const { return m_pTangibleLight; }

	virtual bool RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist);

	// ���ڱ������Primitive�������
	virtual void SampleForArea(Vector3& o_pos, Vector3& o_norm, float& o_pdfA);
	virtual float GetPdfArea() { return 1.0f / GetSurfaceArea(); }

	// ��������Ƕ�Primitive�������
	virtual void SampleForSolidAngle(const NXHit& hitInfo, Vector3& o_pos, Vector3& o_norm, float& o_pdfW);
	virtual float GetPdfSolidAngle(const NXHit& hitInfo, const Vector3& posLight, const Vector3& normLight, const Vector3& dirLight);

protected:
	void InitVertexIndexBuffer();
	void InitAABB();

private:
	NXTriangle SampleTriangle();		// �������PDF������һ������

protected:
	ID3D11Buffer*						m_pVertexBuffer;
	ID3D11Buffer*						m_pIndexBuffer;
	//ID3D11Texture2D*					m_pTexture;
	ID3D11ShaderResourceView*			m_pTextureSRV;

	std::vector<VertexPNT>				m_vertices;
	std::vector<USHORT>					m_indices;
	std::vector<Vector3>				m_points;	// vertices position ����

	ConstantBufferMaterial				m_cbDataMaterial;
	ID3D11Buffer*						m_cbMaterial;
	std::shared_ptr<NXPBRMaterial>		m_pPBRMaterial;

	std::shared_ptr<NXPBRTangibleLight>	m_pTangibleLight;	// ���Խ�Primitive����Ϊ��Դ
	std::shared_ptr<NXPrimitive>		m_pThis;

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
	NXTriangle(const std::shared_ptr<NXPrimitive>& pShape, int startIndex);
	~NXTriangle() {};

	float Area() const;
	VertexPNT GetPointData(int PointId) const;
	bool RayCast(const Ray& localRay, NXHit& outHitInfo, float& outDist);

private:
	int startIndex;
	std::shared_ptr<NXPrimitive> pShape;
};
