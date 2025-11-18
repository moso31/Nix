#pragma once
#include "BaseDefs/CppSTLFully.h"
#include "BaseDefs/DX12.h"
#include "NXConvertString.h"
#include "DirectXTex.h"

namespace NXConvert
{

std::string GetPathOfImguiIni();

// 2023.5.31
// 检测是否是 UNORM 格式的纹理
bool IsUnormFormat(DXGI_FORMAT fmt);

// 【WTF】我当时是为什么写了下面这个 SafeDXGIFormat ????? 
// 完 全 失 忆
// ……
// 2023.5.31
// 对读入的 UNORM 格式的纹理，进行安全化格式转换。
// UNORM 类型的纹理需要遵照以下规则进行处理：
// 1. 对R, RG, RGB格式的UNORM纹理，转成8位的RGBA_UNORM;
// 2. 对非8位的UNORM纹理，转成8位的RGBA_UNORM;
// 3. 保留SRGB尾缀
// 其它类型纹理暂时不需要进行处理
DXGI_FORMAT SafeDXGIFormat(DXGI_FORMAT fmt);

// 2024.4.9
// 从一个DXGI_FORMAT中去掉Typeless类型
// DX12建议创建资源时指定clearValue，而clearValue的格式不能是Typeless类型
DXGI_FORMAT DXGINoTypeless(DXGI_FORMAT fmt, bool isDepthStencil = false);

// 2024.6.22
// 从一个fmt格式转换成DSVFormat可以使用的格式
// 如果fmt不是DepthStencil支持的类型，返回DXGI_FORMAT_UNKNOWN
DXGI_FORMAT TypelessToDSVFormat(DXGI_FORMAT fmt);

DXGI_FORMAT ForceSRGB(DXGI_FORMAT fmt);
DXGI_FORMAT ForceLinear(DXGI_FORMAT fmt);

// 2025.9.2 是否是BC压缩格式
bool IsBCFormat(DXGI_FORMAT fmt);

// 2025.11.18 将DXGI_FORMAT转换为字符串(省略DXGI_FORMAT_前缀)
std::string GetDXGIFormatString(DXGI_FORMAT fmt);

bool IsImageFileExtension(const std::string& strExt);
bool IsDDSFileExtension(const std::string& strExt);
bool IsRawFileExtension(const std::string& strExt);
bool IsMaterialFileExtension(const std::string& strExt);
bool IsDiffuseProfileExtension(const std::string& strExt);
bool IsTerrainLayerExtension(const std::string& strExt);

bool IsMaterialDefaultPath(const std::string& s);

void GetMipSliceFromLayoutIndex(int layoutIndex, int mipSize, int sliceSize, int& oMip, int& oSlice);

bool GetMetadataFromFile(const std::filesystem::path& path, DirectX::TexMetadata& oMetaData);

}