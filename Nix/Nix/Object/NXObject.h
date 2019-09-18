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

	virtual void Update();

protected:
	string m_name;
	vector<shared_ptr<NXScript>> m_scripts;
};