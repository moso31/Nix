#pragma once
#include "BaseDefs/CppSTLFully.h"
#include "BaseDefs/DX12.h"

namespace NXConvert
{

//  wstring -> string 
std::string ws2s(const std::wstring& ws);

// string -> wstring
std::wstring s2ws(const std::string& s);

std::string s2lower(const std::string& s);
std::wstring s2lower(const std::wstring& s);

std::string Trim(std::string& str);

std::string GetPathOfImguiIni();

// 从一段函数文本中提取出该函数的首行。这个主要是用于 HLSL 编译的
std::string GetTitleOfFunctionData(const std::string& functionString);

// 2023.5.31
// 检测是否是 UNORM 格式的纹理
bool IsUnormFormat(DXGI_FORMAT fmt);

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

DXGI_FORMAT ForceSRGB(DXGI_FORMAT fmt);
DXGI_FORMAT ForceLinear(DXGI_FORMAT fmt);

bool IsImageFileExtension(const std::string& strExt);
bool IsMaterialFileExtension(const std::string& strExt);
bool IsDiffuseProfileExtension(const std::string& strExt);

void getline_safe(std::ifstream& ifs, std::string& s);

bool IsMaterialDefaultPath(const std::string& s);

}