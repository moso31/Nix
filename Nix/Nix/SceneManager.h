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
#include "NXLight.h"
#include "NXMaterial.h"

#include "NXPBRLight.h"
#include "NXPBRMaterial.h"

class SceneManager
{
public:
	SceneManager();
	SceneManager(const shared_ptr<NXScene>& pScene);
	~SceneManager();

	shared_ptr<NXScript>	CreateScript(const NXScriptType scriptType, const shared_ptr<NXObject>& pObject);
	shared_ptr<NXListener>	AddEventListener(const NXEventType eventType, const shared_ptr<NXObject>& pObject, const function<void(NXEventArg)>& pFunc);

	shared_ptr<NXBox> CreateBox(const string& name, const float width, const float height, const float length, const shared_ptr<NXMaterial>& material, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	shared_ptr<NXSphere> CreateSphere(const string& name, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const shared_ptr<NXMaterial>& material, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	shared_ptr<NXCylinder> CreateCylinder(const string& name, const float radius, const float length, const UINT segmentCircle, const UINT segmentLength, const shared_ptr<NXMaterial>& material, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	shared_ptr<NXPlane> CreatePlane(const string& name, const float width, const float height, const shared_ptr<NXMaterial>& material, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	bool CreateFBXMeshes(const string& filePath, const shared_ptr<NXMaterial>& pDefaultMaterial, vector<shared_ptr<NXMesh>>& outMeshes);

	shared_ptr<NXDirectionalLight> CreateDirectionalLight(const string& name, const Vector4& ambient, const Vector4& diffuse, const Vector3& specular, const float specularW, const Vector3& direction);
	shared_ptr<NXPointLight> CreatePointLight(const string& name, const Vector4& ambient, const Vector4& diffuse, const Vector3& specular, const float specularW, const Vector3& position, const float range, const Vector3& attenuation);
	shared_ptr<NXSpotLight> CreateSpotLight(const string& name, const Vector4& ambient, const Vector4& diffuse, const Vector3& specular, const float specularW, const Vector3& position, const float range, const Vector3& direction, const float spot, const Vector3& attenuation);

	shared_ptr<NXCamera> CreateCamera(const string& name, const float FovY, const float zNear, const float zFar, const Vector3& eye, const Vector3& at, const Vector3& up);

	shared_ptr<NXMaterial> CreateMaterial(const string& name, const Vector4& ambient = Vector4(0.5f, 0.5f, 0.5f, 0.5f), const Vector4& diffuse = Vector4(0.5f, 0.5f, 0.5f, 0.5f), const Vector4& specular = Vector4(0.5f, 0.5f, 0.5f, 0.5f), const float opacity = 1.0f, const Vector4& reflect = Vector4(0.0f));

	shared_ptr<NXPBRMaterial> CreatePBRMatte(const Vector3& diffuse);
	shared_ptr<NXPBRMaterial> CreatePBRGlass(const Vector3& diffuse, float IOR = 1.5);
	shared_ptr<NXPBRMaterial> CreatePBRMirror(const Vector3& diffuse);

	shared_ptr<NXPBRLight> CreatePBRPointLight(const Vector3& position, const Vector3 &intensity);

	// 绑定Outline父子关系
	bool BindParent(shared_ptr<NXObject> pParent, shared_ptr<NXObject> pChild);

private:
	shared_ptr<NXScene> m_scene;
};
