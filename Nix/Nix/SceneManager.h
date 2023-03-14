#pragma once
#include <filesystem>

#include "NXInstance.h"
#include "NXScriptType.h"

#include "HBVH.h"

#include "NXCamera.h"
#include "NXPBRLight.h"
#include "NXCubeMap.h"
#include "NXPBRMaterial.h"

const std::string g_defaultTex_white_str = ".\\Resource\\white1x1.png";
const std::string g_defaultTex_normal_str = ".\\Resource\\normal1x1.png";
const std::wstring g_defaultTex_white_wstr = L".\\Resource\\white1x1.png";
const std::wstring g_defaultTex_normal_wstr = L".\\Resource\\normal1x1.png";

enum NXPlaneAxis;

class NXScene;
class SceneManager : public NXInstance<SceneManager>
{
public:
	explicit SceneManager() = default;
	~SceneManager() {}

	static void SetWorkingScene(NXScene* pScene);

	static void BuildBVHTrees(const HBVHSplitMode SplitMode);

	static NXScript* CreateScript(const NXScriptType scriptType, NXObject* pObject);

	static NXPrimitive* CreateBox(const std::string name, const float width, const float height, const float length, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	static NXPrimitive* CreateSphere(const std::string name, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	static NXPrimitive* CreateSHSphere(const std::string name, const int l, const int m, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	static NXPrimitive* CreateCylinder(const std::string name, const float radius, const float length, const UINT segmentCircle, const UINT segmentLength, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	static NXPrimitive* CreatePlane(const std::string name, const float width, const float height, const NXPlaneAxis axis, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	static NXPrefab* CreateFBXPrefab(const std::string name, const std::string filePath, bool bAutoCalcTangents = true);

	static NXCamera* CreateCamera(const std::string name, const float FovY, const float zNear, const float zFar, const Vector3& eye, const Vector3& at, const Vector3& up);

	static NXMaterial* LoadFromNmatFile(const std::filesystem::path& matFilePath);

	static NXPBRMaterialStandard* CreatePBRMaterialStandard(const std::string name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao,
		const std::wstring albedoTexFilePath = g_defaultTex_white_wstr,
		const std::wstring normalTexFilePath = g_defaultTex_normal_wstr,
		const std::wstring metallicTexFilePath = g_defaultTex_white_wstr,
		const std::wstring roughnessTexFilePath = g_defaultTex_white_wstr,
		const std::wstring aoTexFilePath = g_defaultTex_white_wstr, 
		const std::string& folderPath = "");
	static NXPBRMaterialTranslucent* CreatePBRMaterialTranslucent(const std::string name, const Vector3& albedo, const Vector3& normal, const float metallic, const float roughness, const float ao, const float opacity,
		const std::wstring albedoTexFilePath = g_defaultTex_white_wstr,
		const std::wstring normalTexFilePath = g_defaultTex_normal_wstr,
		const std::wstring metallicTexFilePath = g_defaultTex_white_wstr,
		const std::wstring roughnessTexFilePath = g_defaultTex_white_wstr,
		const std::wstring aoTexFilePath = g_defaultTex_white_wstr, 
		const std::string& folderPath = "");

	static void BindMaterial(NXRenderableObject* pRenderableObj, NXMaterial* pMaterial);
	static void BindMaterial(NXSubMeshBase* pSubMesh, NXMaterial* pMaterial);

	// 根据类型重新创建某个材质。
	// GUI中更改材质类型时，会使用此逻辑。
	static void ReTypeMaterial(NXMaterial* pSrcMaterial, NXMaterialType destMaterialType);

	static NXPBRDistantLight* CreatePBRDistantLight(const Vector3& direction, const Vector3& color, const float illuminance);
	static NXPBRPointLight* CreatePBRPointLight(const Vector3& position, const Vector3& color, const float intensity, const float influenceRadius);
	static NXPBRSpotLight* CreatePBRSpotLight(const Vector3& position, const Vector3& direction, const Vector3& color, const float intensity, const float innerAngle, const float outerAngle, const float influenceRadius);
	static NXCubeMap* CreateCubeMap(const std::string name, const std::filesystem::path& filePath);

	// 绑定Outline父子关系
	static bool BindParent(NXObject* pParent, NXObject* pChild);

	static void Release();

private:
	static void RegisterCubeMap(NXCubeMap* newCubeMap);
	static void RegisterPrimitive(NXPrimitive* newPrimitive, NXObject* pParent = nullptr);
	static void RegisterPrefab(NXPrefab* newPrefab, NXObject* pParent = nullptr);
	static void RegisterCamera(NXCamera* newCamera, bool isMainCamera, NXObject* pParent = nullptr);
	static void RegisterMaterial(NXMaterial* newMaterial);
	static void RegisterLight(NXPBRLight* newLight, NXObject* pParent = nullptr);

	static NXMaterial* FindMaterial(size_t matPathHash);
	static NXMaterial* LoadStandardPBRMaterialFromFile(std::ifstream& ifs, const std::string& matName, const std::string& matFilePath);
	static NXMaterial* LoadTranslucentPBRMaterialFromFile(std::ifstream& ifs, const std::string& matName, const std::string& matFilePath);

private:
	static void getline_safe(std::ifstream& ifs, std::string& s);
	static bool IsDefaultPath(const std::string& s);

private:
	static NXScene* s_pWorkingScene;
};
