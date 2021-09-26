#pragma once
#include "NXEvent.h"
#include "NXScriptType.h"

#include "HBVH.h"

#include "NXCamera.h"
#include "NXPBRLight.h"
#include "NXCubeMap.h"
#include "NXPBRMaterial.h"

enum NXPlaneAxis;

class NXScene;
class SceneManager
{
public:
	SceneManager();
	SceneManager(NXScene* pScene);
	~SceneManager();

	void BuildBVHTrees(const HBVHSplitMode SplitMode);

	NXScript* CreateScript(const NXScriptType scriptType, NXObject* pObject);

	NXPrimitive* CreateBox(const std::string name, const float width, const float height, const float length, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	NXPrimitive* CreateSphere(const std::string name, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	NXPrimitive* CreateCylinder(const std::string name, const float radius, const float length, const UINT segmentCircle, const UINT segmentLength, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	NXPrimitive* CreatePlane(const std::string name, const float width, const float height, const NXPlaneAxis axis, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	bool CreateFBXMeshes(const std::string filePath, std::vector<NXPrimitive*>& outMeshes, bool bAutoCalcTangents = true);

	NXCamera* CreateCamera(const std::string name, const float FovY, const float zNear, const float zFar, const Vector3& eye, const Vector3& at, const Vector3& up);
	NXPBRMaterial* CreatePBRMaterial(const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao);
	NXPBRPointLight* CreatePBRPointLight(const Vector3& position, const Vector3& intensity);
	NXPBRDistantLight* CreatePBRDistantLight(const Vector3& direction, const Vector3& radiance);
	NXPBREnvironmentLight* CreatePBREnvironmentLight(NXCubeMap* pCubeMap, const Vector3& Intensity = Vector3(1.0f));
	NXCubeMap* CreateCubeMap(const std::string name, const std::wstring filePath);

	// 绑定Outline父子关系
	bool BindParent(NXObject* pParent, NXObject* pChild);

	void Release();

private:
	void RegisterCubeMap(NXCubeMap* newCubeMap);
	void RegisterPrimitive(NXPrimitive* newPrimitive, NXObject* pParent = nullptr);
	void RegisterCamera(NXCamera* newCamera, bool isMainCamera, NXObject* pParent = nullptr);
	void RegisterMaterial(NXPBRMaterial* newMaterial);
	void RegisterLight(NXPBRLight* newLight, NXObject* pParent = nullptr);

private:
	friend NXScene;
	NXScene* m_pScene;

	// 求交加速结构（用于NXRayTracer的射线检测）
	HBVHTree* m_pBVHTree;

	// 隐藏的根object
	NXObject* m_pRootObject;
	std::vector<NXObject*> m_objects;
	//std::vector<NXScript*> m_scripts;

	std::vector<NXPrimitive*> m_primitives;
	std::set<NXPBRMaterial*> m_pbrMaterials;
	std::vector<NXPBRLight*> m_pbrLights;

	NXCamera* m_pMainCamera;
	NXCubeMap* m_pCubeMap;
};
