#include "NXEditorObjectManager.h"
#include "NXScene.h"
#include "NXPrimitive.h"
#include "NXSubMeshGeometryEditor.h"
#include "NXCamera.h"

NXEditorObjectManager::NXEditorObjectManager(NXScene* pScene) :
	m_pScene(pScene)
{
}

NXEditorObjectManager::~NXEditorObjectManager()
{
}

void NXEditorObjectManager::Init()
{
	NXPrimitive* pObj = new NXPrimitive();
	pObj->SetName("game_move_arrows");
	NXSubMeshGeometryEditor::GetInstance()->CreateMoveArrows(pObj);

	m_editorObjs.push_back(pObj);

	InitBoundingStructures();

	// EditorObjects默认不可见
	SetVisible(false);
}

void NXEditorObjectManager::InitBoundingStructures()
{
	for (auto pMesh : m_editorObjs) pMesh->InitAABB();
}

void NXEditorObjectManager::UpdateTransform()
{
	Vector3 camPos = m_pScene->GetMainCamera()->GetTranslation();
	for (auto pObj : m_editorObjs)
	{
		pObj->UpdateTransform();

		// 2022.9.26 临时的 Translator rescale，距离越远scale越大。
		// 使其始终在屏幕上能保持固定大小。
		float dist = Vector3::Distance(camPos, pObj->GetTranslation());
		pObj->SetScale(Vector3(dist * 0.1f));
	}
}

void NXEditorObjectManager::Release()
{
	for (auto pEditorObj : m_editorObjs) SafeRelease(pEditorObj);
}

bool NXEditorObjectManager::RayCast(const Ray& worldRay, NXHit& outHitInfo, float tMax)
{
	outHitInfo.Reset();
	float outDist = tMax;

	for (auto pMesh : m_editorObjs)
	{
		if (pMesh->RayCast(worldRay, outHitInfo, outDist))
		{
			// hit.
		}
	}

	if (!outHitInfo.pSubMesh)
		return false;

	//outHitInfo.LocalToWorld();
	return true;
}

void NXEditorObjectManager::MoveTranslatorTo(const Vector3& position)
{
	m_editorObjs[0]->SetTranslation(position);
}

void NXEditorObjectManager::SetVisible(bool val)
{
	for (auto p : m_editorObjs) p->SetVisible(val);
}
