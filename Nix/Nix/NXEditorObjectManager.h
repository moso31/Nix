#pragma once
#include "Header.h"

class NXHit;
// �༭�����壨=�ƶ���ͷ����ת�� ���������Mesh���Ĺ�����
class NXEditorObjectManager
{
public:
	NXEditorObjectManager(NXScene* pScene);
	~NXEditorObjectManager();

	void Init();
	void InitBoundingStructures();
	void UpdateTransform();
	void Release();
	
	std::vector<NXPrimitive*> GetEditableObjects() { return m_editorObjs; }

	bool RayCast(const Ray& ray, NXHit& outHitInfo, float tMax = FLT_MAX);

	EditorObjectID	GetHighLightID() { return m_uHighLightID; }
	void			SetHighLightID(EditorObjectID value) { m_uHighLightID = value; }
	void			MoveTranslatorTo(const Vector3& position);

private:
	NXScene* m_pScene;
	std::vector<NXPrimitive*> m_editorObjs;

	// ���ڿ���EditorObject-SubMesh-�Ƿ����
	EditorObjectID m_uHighLightID;
};
