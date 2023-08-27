#pragma once
#include <list>
#include <vector>
#include <string>

class NXTransform;
class NXRenderableObject;
class NXPrimitive;
class NXPrefab;
class NXScript;
class NXObject
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
	virtual NXPrefab* IsPrefab() { return nullptr; }

	void AddScript(NXScript* script);
	std::vector<NXScript*> GetScripts();

	NXObject* GetParent();
	void SetParent(NXObject* pParent);

	size_t GetChildCount();
	std::list<NXObject*> GetChilds();
	void RemoveChild(NXObject* pObject);

	// ���pObject�Ƿ��ǵ�ǰ�ڵ���ӽڵ㡣
	bool IsChild(NXObject* pObject);

	virtual void Update();
	virtual void Release();

protected:
	std::string m_name;
	std::vector<NXScript*> m_scripts;
	
	NXObject* m_parent = nullptr;
	std::list<NXObject*> m_childs;
};