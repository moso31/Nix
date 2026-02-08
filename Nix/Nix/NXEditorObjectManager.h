#pragma once
#include "BaseDefs/Math.h"

enum EditorObjectID;
class NXHit;
class NXScene;
class NXPrimitive;
// 编辑器物体（=移动箭头，旋转轴 这类物体的Mesh）的管理类
class NXEditorObjectManager
{
public:
	NXEditorObjectManager(NXScene* pScene);
	virtual ~NXEditorObjectManager();

	void Init();
	void InitBoundingStructures();
	void UpdateTransform();
	void Release();
	
	std::vector<NXPrimitive*> GetEditableObjects() { return m_editorObjs; }

	bool RayCast(const Ray& ray, NXHit& outHitInfo, float tMax = FLT_MAX);

	EditorObjectID	GetHighLightID() { return m_uHighLightID; }
	void			SetHighLightID(EditorObjectID value) { m_uHighLightID = value; }

	// "MoveArrow" , "Translator" is same meaning.
	NXPrimitive*	GetMoveArrow() { return m_editorObjs[0]; }
	void			MoveTranslatorTo(const Vector3& position);

	void			SetVisible(bool val);

private:
	NXScene* m_pScene;
	std::vector<NXPrimitive*> m_editorObjs;

	// 用于控制EditorObject-SubMesh-是否高亮
	EditorObjectID m_uHighLightID;
};
