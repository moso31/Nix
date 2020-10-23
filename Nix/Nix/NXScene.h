#pragma once
#include "NXObject.h"
#include "NXPBRMaterial.h"
#include "NXPBRLight.h"
#include "HBVH.h"

// temp include.
#include "ShaderStructures.h"

class NXScene : public NXObject
{
public:
	NXScene();
	~NXScene();

	void OnMouseDown(NXEventArg eArg);
	void OnKeyDown(NXEventArg eArg);

	void Init();
	void InitScripts();
	void UpdateTransform(std::shared_ptr<NXObject> pObject = nullptr);
	void UpdateScripts();
	void UpdateCamera();
	void Release();

	std::shared_ptr<NXCamera> GetMainCamera() { return m_pMainCamera; }

	// 场景-射线相交测试
	bool RayCast(const Ray& ray, NXHit& out_hitInfo, float tMax = FLT_MAX);

	BoundingSphere	GetBoundingSphere() { return m_boundingSphere; }
	AABB GetAABB() { return m_aabb; }
	std::vector<std::shared_ptr<NXPrimitive>> GetPrimitives() { return m_primitives; }
	std::shared_ptr<NXCubeMap> GetCubeMap() { return m_pCubeMap; }

	// PBR场景数据
	std::vector<std::shared_ptr<NXPBRLight>> GetPBRLights() { return m_pbrLights; }
	std::vector<std::shared_ptr<NXPBRMaterial>>	GetPBRMaterials() { return m_pbrMaterials; }

	// 目前只对第一个光源创建Parallel ShadowMap。
	void InitShadowMapTransformInfo(ConstantBufferShadowMapTransform& out_cb);

	ID3D11Buffer* GetConstantBufferLights() const { return m_cbLights; }

	// 更新场景BVH树
	void BuildBVHTrees(const HBVHSplitMode SplitMode);
private:
	void InitBoundingStructures();

private:
	friend SceneManager;
	std::shared_ptr<SceneManager> m_sceneManager;

	// 求交加速结构（用于NXRayTracer的射线检测）
	std::shared_ptr<HBVHTree> m_pBVHTree;

	// 隐藏的根object
	std::shared_ptr<NXObject> m_pRootObject;

	// object : light、camera、primitive均属于object。
	std::vector<std::shared_ptr<NXObject>> m_objects;
	std::vector<std::shared_ptr<NXPBRLight>> m_lights;
	std::vector<std::shared_ptr<NXPrimitive>> m_primitives;
	std::shared_ptr<NXCamera> m_pMainCamera;

	// PBR材质
	std::vector<std::shared_ptr<NXPBRMaterial>> m_pbrMaterials;

	// 灯光
	std::vector<std::shared_ptr<NXPBRLight>> m_pbrLights;
	ID3D11Buffer* m_cbLights;
	ConstantBufferLight m_cbDataLights;
	std::shared_ptr<NXCubeMap> m_pCubeMap;

	AABB m_aabb;
	BoundingSphere m_boundingSphere;
};
