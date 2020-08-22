#pragma once
#include "Header.h"

class NXObject : public std::enable_shared_from_this<NXObject>
{
public:
	NXObject();
	~NXObject();

	std::string GetName();
	void SetName(std::string name);

	void AddScript(const std::shared_ptr<NXScript> &script);
	std::vector<std::shared_ptr<NXScript>> GetScripts();

	std::shared_ptr<NXObject> GetParent();
	void SetParent(std::shared_ptr<NXObject> pParent);

	size_t GetChildCount();
	std::list<std::shared_ptr<NXObject>> GetChilds();
	void RemoveChild(const std::shared_ptr<NXObject>& pObject);

	// 检测pObject是否是当前节点的子节点。
	bool IsChild(std::shared_ptr<NXObject> pObject);

	virtual void Update();
	virtual void Release();

protected:
	std::string m_name;
	std::vector<std::shared_ptr<NXScript>> m_scripts;
	
private:
	std::shared_ptr<NXObject> m_parent;
	std::list<std::shared_ptr<NXObject>> m_childs;
};