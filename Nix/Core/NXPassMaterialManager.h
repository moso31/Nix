#pragma once
#include <map>
#include <string>
#include <vector>

#define NXPassMng NXPassMaterialManager::GetInstance()

class NXPassMaterial;
struct NXPassMaterialRemoving
{
	NXPassMaterial* pMaterial;
	uint64_t fenceValue;
};

class NXPassMaterialManager
{
public:
	static NXPassMaterialManager* GetInstance();

	void InitDefaultRenderer();
	void Release();

	void AddMaterial(const std::string& name, NXPassMaterial* pMat, bool bCompile);
	void RemoveMaterial(const std::string& name);

	void FrameCleanup();

	NXPassMaterial* GetPassMaterial(const std::string& name);
	std::map<std::string, NXPassMaterial*>& GetPassMaterialMaps() { return m_pPassMaterialMaps; }

private:
	NXPassMaterialManager() = default;
	~NXPassMaterialManager() = default;

	std::map<std::string, NXPassMaterial*> m_pPassMaterialMaps;

	std::vector<NXPassMaterialRemoving> m_removingMaterials;
};
