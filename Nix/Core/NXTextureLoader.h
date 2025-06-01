#pragma once
#include <filesystem>
#include <mutex>
#include "NXInstance.h"
#include "NXTextureDefinitions.h"
#include "DirectXTex.h"
using namespace DirectX;

struct NXTextureLoaderTaskResult
{
	TexMetadata metadata;
	std::shared_ptr<ScratchImage> pImage;
};

struct NXTextureLoaderTask
{
	std::filesystem::path path;
	NXResourceType type;
	NXTextureSerializationData serializationData;

	std::function<void(NXTextureLoaderTaskResult)> pCallBack;
};

class NXTextureLoader : public NXInstance<NXTextureLoader>
{
public:
	NXTextureLoader();
	~NXTextureLoader();

	void AddTask(const NXTextureLoaderTask& task);

	void Update();

private:
	void DoTask(const NXTextureLoaderTask& task);

private:
	std::mutex m_mutex;
	std::vector<NXTextureLoaderTask> m_tasks;
};
