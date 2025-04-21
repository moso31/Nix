#pragma once
#include <list>
#include <string>
#include "BaseDefs/DX12.h"
#include "NXRefCountable.h"

class NXTransform;
class NXRenderableObject;
class NXPrimitive;
class NXPrefab;
class NXTerrain;
class NXScript;
class NXTexture;
class NXTexture2D;

class NXObject : public NXRefCountable
{
public:
	NXObject() {}
	NXObject(const std::string& name) : m_name(name) {}
	virtual ~NXObject() {}

	std::string GetName() { return m_name; }
	void SetName(std::string name) { m_name = name; }

	virtual NXTransform* IsTransform() { return nullptr; }
	virtual NXRenderableObject* IsRenderableObject() { return nullptr; }
	virtual NXPrimitive* IsPrimitive() { return nullptr; }
	virtual NXTerrain* IsTerrain() { return nullptr; }
	virtual NXPrefab* IsPrefab() { return nullptr; }

	NXObject* GetParent();
	void SetParent(NXObject* pParent);

	size_t GetChildCount();
	std::list<NXObject*> GetChilds();
	void RemoveChild(NXObject* pObject);

	// 检测pObject是否是当前节点的子节点。
	bool IsChild(NXObject* pObject);

	virtual void Update(ID3D12GraphicsCommandList* pCmdList) {};
	virtual void Release();

protected:
	std::string m_name;
	
	NXObject* m_parent = nullptr;
	std::list<NXObject*> m_childs;
};