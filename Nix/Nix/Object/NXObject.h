#pragma once
#include "Header.h"

class NXObject : public enable_shared_from_this<NXObject>
{
public:
	NXObject();
	~NXObject();

	string GetName();
	void SetName(string name);

	void AddScript(const shared_ptr<NXScript> &script);
	vector<shared_ptr<NXScript>> GetScripts();

	shared_ptr<NXObject> GetParent();
	void SetParent(shared_ptr<NXObject> pParent);
	void ClearParent();

	size_t GetChildCount();
	list<shared_ptr<NXObject>> GetChilds();
	void RemoveChild(const shared_ptr<NXObject>& pObject);

	// 检测pObject是否是当前节点的子节点。
	bool IsChild(shared_ptr<NXObject> pObject);

	virtual void Update();
	virtual void Release();

protected:
	string m_name;
	vector<shared_ptr<NXScript>> m_scripts;
	
private:
	shared_ptr<NXObject> m_parent;
	list<shared_ptr<NXObject>> m_childs;
};