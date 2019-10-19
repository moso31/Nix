#pragma once
#include "Header.h"

class NXObject : public enable_shared_from_this<NXObject>
{
public:
	NXObject() = default;
	~NXObject();

	string GetName();
	void SetName(string name);

	void AddScript(const shared_ptr<NXScript> &script);
	vector<shared_ptr<NXScript>> GetScripts();

	shared_ptr<NXObject> GetParent();
	void SetParent(shared_ptr<NXObject> pParent);

	size_t GetChildCount();
	vector<shared_ptr<NXObject>> GetChilds();
	shared_ptr<NXObject> GetChild(size_t index);

	virtual void Update();
	virtual void Release();

protected:
	string m_name;
	vector<shared_ptr<NXScript>> m_scripts;
	
private:
	shared_ptr<NXObject> m_parent;
	vector<shared_ptr<NXObject>> m_childs;
};