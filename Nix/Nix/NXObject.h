#pragma once
#include "Header.h"

class NXObject
{
public:
	NXObject();
	virtual ~NXObject();

	std::string GetName();
	void SetName(std::string name);

	void AddScript(NXScript* script);
	std::vector<NXScript*> GetScripts();

	NXObject* GetParent();
	void SetParent(NXObject* pParent);

	size_t GetChildCount();
	std::list<NXObject*> GetChilds();
	void RemoveChild(NXObject* pObject);

	// 检测pObject是否是当前节点的子节点。
	bool IsChild(NXObject* pObject);

	virtual void Update();
	virtual void Release();

protected:
	std::string m_name;
	std::vector<NXScript*> m_scripts;
	
private:
	NXObject* m_parent;
	std::list<NXObject*> m_childs;
};