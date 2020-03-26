#pragma once
#include "NXObject.h"
#include "NXMaterial.h"

// temp include.
#include "ShaderStructures.h"

class NXScene : public NXObject
{
public:
	NXScene();
	~NXScene();

	void OnMouseDown(NXEventArg eArg);

	void Init();
	void InitScripts();
	void UpdateTransform(shared_ptr<NXObject> pObject = nullptr);
	void UpdateScripts();
	void UpdateCamera();
	void Release();

	BoundingSphere						GetBoundingSphere()		{ return m_boundingSphere; }
	AABB								GetAABB()				{ return m_aabb; }
	vector<shared_ptr<NXPrimitive>>		GetPrimitives()			{ return m_primitives; }

	// PBR��������
	vector<shared_ptr<NXPBRPointLight>>	GetPBRLights()			{ return m_pbrLights; }
	vector<shared_ptr<NXPBRMaterial>>	GetPBRMaterials()		{ return m_pbrMaterials; }

	// Ŀǰֻ�Ե�һ����Դ����Parallel ShadowMap��
	void InitShadowMapTransformInfo(ConstantBufferShadowMapTransform& out_cb);

	ID3D11Buffer* GetConstantBufferLights() const { return m_cbLights; }

private:
	void InitBoundingStructures();

private:
	friend SceneManager;
	shared_ptr<SceneManager> m_sceneManager;

	// ���صĸ�object
	shared_ptr<NXObject> m_pRootObject;

	// object : light��camera��primitive������object��
	vector<shared_ptr<NXObject>> m_objects;
	vector<shared_ptr<NXLight>> m_lights;
	vector<shared_ptr<NXPrimitive>> m_primitives;
	shared_ptr<NXCamera> m_mainCamera;

	// ����
	vector<shared_ptr<NXMaterial>> m_materials;

	ID3D11Buffer* m_cbLights;
	ConstantBufferLight m_cbDataLights;

	AABB m_aabb;
	BoundingSphere m_boundingSphere;

	// PBR����
	vector<shared_ptr<NXPBRPointLight>> m_pbrLights;
	vector<shared_ptr<NXPBRMaterial>> m_pbrMaterials;
};
