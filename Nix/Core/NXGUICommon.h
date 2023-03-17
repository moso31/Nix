#pragma once
#include <filesystem>

enum NXTextureType;
struct NXTextureInfoData
{
	NXTextureInfoData();
	NXTextureType eTexType;
	bool bSRGB;
	bool bInvertNormalY;
	bool bGenerateMipMap;
	bool bCubeMap;
};

class NXTexture;
class NXMaterial;
namespace NXGUICommon
{
	void CreateDefaultMaterialFile(const std::filesystem::path& path, const std::string& matName);
	void SaveMaterialFile(NXMaterial* pMaterial);

	// 读取纹理的*.nxInfo信息。
	// 注意！path是纹理的Path，而非nxInfo文件的Path
	NXTextureInfoData LoadTextureInfoFile(const std::filesystem::path& path);
	void SaveTextureInfoFile(NXTexture* pTexture, const NXTextureInfoData& info);
}
