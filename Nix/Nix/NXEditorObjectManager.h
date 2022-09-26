#pragma once
#include "Header.h"

class NXHit;
// 编辑器物体（=移动箭头，旋转轴 这类物体的Mesh）的管理类
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

	void MoveTranslatorTo(const Vector3& position);

private:
	NXScene* m_pScene;
	std::vector<NXPrimitive*> m_editorObjs;
};
