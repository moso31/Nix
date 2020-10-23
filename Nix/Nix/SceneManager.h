#pragma once
#include "NXScene.h"
#include "NXScriptType.h"
#include "NXEvent.h"

#include "NXMesh.h"
#include "NXBox.h"
#include "NXSphere.h"
#include "NXCylinder.h"
#include "NXPlane.h"
#include "NXCamera.h"
#include "NXPBRLight.h"
#include "NXPBRMaterial.h"

class SceneManager
{
public:
	SceneManager();
	SceneManager(const std::shared_ptr<NXScene>& pScene);
	~SceneManager();

	std::shared_ptr<NXScript>	CreateScript(const NXScriptType scriptType, const std::shared_ptr<NXObject>& pObject);
	std::shared_ptr<NXListener>	AddEventListener(const NXEventType eventType, const std::shared_ptr<NXObject>& pObject, const std::function<void(NXEventArg)>& pFunc);

	std::shared_ptr<NXBox> CreateBox(const std::string& name, const float width, const float height, const float length, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	std::shared_ptr<NXSphere> CreateSphere(const std::string& name, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	std::shared_ptr<NXCylinder> CreateCylinder(const std::string& name, const float radius, const float length, const UINT segmentCircle, const UINT segmentLength, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	std::shared_ptr<NXPlane> CreatePlane(const std::string& name, const float width, const float height, const NXPlaneAxis axis, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	bool CreateFBXMeshes(const std::string& filePath, const std::shared_ptr<NXPBRMaterial>& pDefaultMaterial, std::vector<std::shared_ptr<NXMesh>>& outMeshes);

	std::shared_ptr<NXCamera> CreateCamera(const std::string& name, const float FovY, const float zNear, const float zFar, const Vector3& eye, const Vector3& at, const Vector3& up);

	std::shared_ptr<NXPBRMaterial> CreatePBRMaterial(const Vector3& albedo, const float metallic, const float roughness, const float reflectivity, const float refractivity, const float IOR);

	std::shared_ptr<NXPBRPointLight> CreatePBRPointLight(const Vector3& position, const Vector3& intensity);
	std::shared_ptr<NXPBRDistantLight> CreatePBRDistantLight(const Vector3& direction, const Vector3& radiance);
	std::shared_ptr<NXPBRTangibleLight> CreatePBRTangibleLight(const std::shared_ptr<NXPrimitive>& pPrimitive, const Vector3& radiance);
	std::shared_ptr<NXPBREnvironmentLight> CreatePBREnvironmentLight(const std::shared_ptr<NXCubeMap>& pCubeMap, const Vector3& Intensity = Vector3(1.0f));

	// 绑定Outline父子关系
	bool BindParent(std::shared_ptr<NXObject> pParent, std::shared_ptr<NXObject> pChild);

private:
	std::shared_ptr<NXScene> m_scene;
};
