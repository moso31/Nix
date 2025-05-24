#include "NXMeshResourceManager.h"
#include "NXScene.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXPrimitive.h"
#include "NXPrefab.h"
#include "NXTerrain.h"
#include "NXTextureReloadTesk.h"

void NXMeshResourceManager::Init(NXScene* pScene)
{
	m_pWorkingScene = pScene;
}

NXPrimitive* NXMeshResourceManager::CreateBox(const std::string& name, const float width, const float height, const float length, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
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

NXPrimitive* NXMeshResourceManager::CreateSphere(const std::string& name, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
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

NXPrimitive* NXMeshResourceManager::CreateSHSphere(const std::string& name, const int l, const int m, const float radius, const UINT segmentHorizontal, const UINT segmentVertical, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
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

NXPrimitive* NXMeshResourceManager::CreateCylinder(const std::string& name, const float radius, const float length, const UINT segmentCircle, const UINT segmentLength, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
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

NXPrimitive* NXMeshResourceManager::CreatePlane(const std::string& name, const float width, const float height, const NXPlaneAxis axis, const Vector3& translation, const Vector3& rotation, const Vector3& scale)
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

NXPrefab* NXMeshResourceManager::CreateFBXPrefab(const std::string& name, const std::string filePath, bool bAutoCalcTangents)
{
	auto p = new NXPrefab();
	p->SetName(name);
	NXSubMeshGeometryEditor::GetInstance()->CreateFBXPrefab(p, filePath, bAutoCalcTangents);
	m_pWorkingScene->RegisterPrefab(p);
	return p;
}

NXTerrain* NXMeshResourceManager::CreateTerrain(const std::string& name, int gridSize, int worldSize)
{
	auto p = new NXTerrain(gridSize, worldSize);
	p->SetName(name);
	NXSubMeshGeometryEditor::GetInstance()->CreateTerrain(p, gridSize, worldSize);
	m_pWorkingScene->RegisterTerrain(p);
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

	NXTerrain* pTerrain = pRenderableObj->IsTerrain();
	if (pTerrain)
	{
		auto pSubMesh = pTerrain->GetSubMesh();

		if (pSubMesh)
		{
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
	NXSubMeshReloadTaskPackage* pTaskData = new NXSubMeshReloadTaskPackage();
	for (auto pSubMesh : m_replacingSubMeshes) 
	{
		if (pSubMesh->GetReloadingState() == NXSubMeshReloadState::Start)
		{
			pSubMesh->SwitchToLoadingMaterial();
			pSubMesh->SetReloadingState(NXSubMeshReloadState::Replacing);

			pTaskData->Push(pSubMesh);
		}

		if (pSubMesh->GetReloadingState() == NXSubMeshReloadState::Finish)
		{
			pSubMesh->SwitchToReplacingMaterial();
			pSubMesh->SetReloadingState(NXSubMeshReloadState::None);
			continue;
		}
	}

	if (pTaskData->IsEmpty())
	{
		SafeDelete(pTaskData);
		return;
	}

	// 使用C++20的std::erase和std::remove_if，
	// 从替换队列中，移除pTaskData正在异步处理的内容
	m_replacingSubMeshes.erase(
		std::remove_if(m_replacingSubMeshes.begin(), m_replacingSubMeshes.end(),
			[&](NXSubMeshBase* subMesh) {
				return std::find(pTaskData->submits.begin(), pTaskData->submits.end(), subMesh) != pTaskData->submits.end();
			}),
		m_replacingSubMeshes.end());

	// 异步加载纹理
	bool bAsync = true;
	if (bAsync)
	{
		auto LoadTextureAsyncTask = LoadMaterialAsync(pTaskData);
		LoadTextureAsyncTask.m_handle.promise().m_callbackFunc = [this, pTaskData]() mutable {
			OnLoadMaterialFinish(pTaskData);
		};
	}
	else
	{
		LoadMaterialSync(pTaskData);
		OnLoadMaterialFinish(std::move(pTaskData));
	}
}

void NXMeshResourceManager::Release()
{
}

NXTextureReloadTask NXMeshResourceManager::LoadMaterialAsync(const NXSubMeshReloadTaskPackage* pTaskData)
{
	co_await NXTextureAwaiter();
	LoadMaterialSync(pTaskData);
}

void NXMeshResourceManager::LoadMaterialSync(const NXSubMeshReloadTaskPackage* pTaskData)
{
	// 生成新材质
	for (auto pSubMesh : pTaskData->submits)
	{
		pSubMesh->m_pReplacingMaterial = NXResourceManager::GetInstance()->GetMaterialManager()->LoadFromNSLFile(pSubMesh->m_strReplacingPath);
	}
}

void NXMeshResourceManager::OnLoadMaterialFinish(const NXSubMeshReloadTaskPackage* pTaskData)
{
	for (auto pSubMesh : pTaskData->submits) 
		pSubMesh->OnReplaceFinish();

	SafeDelete(pTaskData);
}
