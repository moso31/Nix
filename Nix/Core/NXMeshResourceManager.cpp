#include "NXMeshResourceManager.h"
#include "NXScene.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXPrimitive.h"
#include "NXPrefab.h"

void NXMeshResourceManager::SetWorkingScene(NXScene* pScene)
{
	m_pWorkingScene = pScene;
}

NXPrimitive* NXMeshResourceManager::CreateBox(const std::string name, const float width, const float height, const float length, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = new NXPrimitive();
	p->SetName(name);
	NXSubMeshGeometryEditor::GetInstance()->CreateBox(p, width, height, length);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	m_pWorkingScene->RegisterPrimitive(p);
	return p;
}

NXPrimitive* NXMeshResourceManager::CreateSphere(const std::string name, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = new NXPrimitive();
	p->SetName(name);
	NXSubMeshGeometryEditor::GetInstance()->CreateSphere(p, radius, segmentHorizontal, segmentVertical);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	m_pWorkingScene->RegisterPrimitive(p);
	return p;
}

NXPrimitive* NXMeshResourceManager::CreateSHSphere(const std::string name, const int l, const int m, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = new NXPrimitive();
	p->SetName(name);
	NXSubMeshGeometryEditor::GetInstance()->CreateSHSphere(p, l, m, radius, segmentHorizontal, segmentVertical);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	m_pWorkingScene->RegisterPrimitive(p);
	return p;
}

NXPrimitive* NXMeshResourceManager::CreateCylinder(const std::string name, const float radius, const float length, const UINT segmentCircle, const UINT segmentLength, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = new NXPrimitive();
	p->SetName(name);
	NXSubMeshGeometryEditor::GetInstance()->CreateCylinder(p, radius, length, segmentCircle, segmentLength);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	m_pWorkingScene->RegisterPrimitive(p);
	return p;
}

NXPrimitive* NXMeshResourceManager::CreatePlane(const std::string name, const float width, const float height, const NXPlaneAxis axis, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
{
	auto p = new NXPrimitive();
	p->SetName(name);
	NXSubMeshGeometryEditor::GetInstance()->CreatePlane(p, width, height, axis);
	p->SetTranslation(translation);
	p->SetRotation(rotation);
	p->SetScale(scale);
	m_pWorkingScene->RegisterPrimitive(p);
	return p;
}

NXPrefab* NXMeshResourceManager::CreateFBXPrefab(const std::string name, const std::string filePath, bool bAutoCalcTangents)
{
	auto p = new NXPrefab();
	p->SetName(name);
	NXSubMeshGeometryEditor::GetInstance()->CreateFBXPrefab(p, filePath, bAutoCalcTangents);
	m_pWorkingScene->RegisterPrefab(p);
	return p;
}

void NXMeshResourceManager::BindMaterial(NXRenderableObject* pRenderableObj, NXMaterial* pMaterial)
{
	NXPrimitive* pPrimitive = pRenderableObj->IsPrimitive();
	if (pPrimitive)
	{
		for (UINT i = 0; i < pPrimitive->GetSubMeshCount(); i++)
		{
			NXSubMeshBase* pSubMesh = pPrimitive->GetSubMesh(i);
			BindMaterial(pSubMesh, pMaterial);
		}
	}

	for (auto pChild : pRenderableObj->GetChilds())
	{
		if (pChild->IsRenderableObject())
		{
			NXRenderableObject* pChildObj = static_cast<NXRenderableObject*>(pChild);
			BindMaterial(pChildObj, pMaterial);
		}
	}
}

void NXMeshResourceManager::BindMaterial(NXSubMeshBase* pSubMesh, NXMaterial* pMaterial)
{
	auto pOldMat = pSubMesh->GetMaterial();
	if (pOldMat)
		pOldMat->RemoveSubMesh(pSubMesh);

	pSubMesh->SetMaterial(pMaterial);
	pMaterial->AddSubMesh(pSubMesh);
}

void NXMeshResourceManager::OnReload()
{
}

void NXMeshResourceManager::Release()
{
}
