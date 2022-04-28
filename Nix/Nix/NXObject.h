#pragma once
#include "Header.h"

// ö�����͡�2021.2.16
// ���ڽ���NXObject���͵������͵�ӳ�䡣
enum NXType
{
	eNone,

	eScene,

	ePrimitive,
	ePrefab,
	eCamera,
	eCubeMap,

	eMax
};

class NXObject
{
public:
	NXObject();
	virtual ~NXObject();

	std::string GetName() { return m_name; }
	void SetName(std::string name) { m_name = name; }

	NXType GetType() { return m_type; }

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
	NXType m_type;
	
private:
	NXObject* m_parent;
	std::list<NXObject*> m_childs;
};