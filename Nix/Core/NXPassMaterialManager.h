#pragma once
#include <map>
#include <string>

#define NXPassMng NXPassMaterialManager::GetInstance()

class NXPassMaterial;

class NXPassMaterialManager
{
public:
	static NXPassMaterialManager* GetInstance();

	void Init();
	void Release();

	NXPassMaterial* GetPassMaterial(const std::string& name);
	std::map<std::string, NXPassMaterial*>& GetPassMaterialMaps() { return m_pPassMaterialMaps; }

private:
	NXPassMaterialManager() = default;
	~NXPassMaterialManager() = default;

	std::map<std::string, NXPassMaterial*> m_pPassMaterialMaps;
};
