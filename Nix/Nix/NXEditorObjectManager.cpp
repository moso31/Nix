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
	pObj->SetName("game_selection_arrows");
	//pObj->SetVisible(false);	// SelectionArrows Ĭ�ϲ��ɼ�
	NXSubMeshGeometryEditor::GetInstance()->CreateSelectionArrows(pObj);

	m_editorObjs.push_back(pObj);

	InitBoundingStructures();
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

		// 2022.9.26 ��ʱ�� Translator rescale������ԽԶscaleԽ��
		// ʹ��ʼ������Ļ���ܱ��̶ֹ���С��
		float dist = Vector3::Distance(camPos, pObj->GetTranslation());
		pObj->SetScale(Vector3(dist * 0.1f));
	}
}

void NXEditorObjectManager::Release()
{
	for (auto pEditorObj : m_editorObjs) SafeRelease(pEditorObj);
}

bool NXEditorObjectManager::RayCast(const Ray& ray, NXHit& outHitInfo, float tMax)
{
	outHitInfo.Reset();
	float outDist = tMax;

	for (auto pMesh : m_editorObjs)
	{
		if (pMesh->RayCast(ray, outHitInfo, outDist))
		{
			// hit.
		}
	}

	if (!outHitInfo.pSubMesh)
		return false;

	outHitInfo.LocalToWorld();
	return true;
}

void NXEditorObjectManager::MoveTranslatorTo(const Vector3& position)
{
	m_editorObjs[0]->SetTranslation(position);
}
