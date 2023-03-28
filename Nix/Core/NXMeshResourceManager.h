#pragma once
#include "NXResourceManagerBase.h"

enum NXPlaneAxis;
class NXMeshResourceManager : public NXResourceManagerBase
{
public:
	NXMeshResourceManager() : m_pWorkingScene(nullptr) {}
	~NXMeshResourceManager() {}

	void SetWorkingScene(NXScene* pScene);

	NXPrimitive* CreateBox(const std::string name, const float width, const float height, const float length, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	NXPrimitive* CreateSphere(const std::string name, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	NXPrimitive* CreateSHSphere(const std::string name, const int l, const int m, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	NXPrimitive* CreateCylinder(const std::string name, const float radius, const float length, const UINT segmentCircle, const UINT segmentLength, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	NXPrimitive* CreatePlane(const std::string name, const float width, const float height, const NXPlaneAxis axis, const Vector3& translation = Vector3(0.f), const Vector3& rotation = Vector3(0.f), const Vector3& scale = Vector3(1.f));
	NXPrefab* CreateFBXPrefab(const std::string name, const std::string filePath, bool bAutoCalcTangents = true);

	void BindMaterial(NXRenderableObject* pRenderableObj, NXMaterial* pMaterial);
	void BindMaterial(NXSubMeshBase* pSubMesh, NXMaterial* pMaterial);

	void OnReload() override;
	void Release() override;

private:
	NXScene* m_pWorkingScene;
};