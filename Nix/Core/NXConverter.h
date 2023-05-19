#pragma once
#include "Header.h"

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

DXGI_FORMAT ForceSRGB(DXGI_FORMAT fmt);
DXGI_FORMAT ForceLinear(DXGI_FORMAT fmt);

bool IsImageFileExtension(const std::string& strFilePath);

bool IsMaterialFileExtension(const std::string& strFilePath);

void getline_safe(std::ifstream& ifs, std::string& s);

bool IsMaterialDefaultPath(const std::string& s);

}