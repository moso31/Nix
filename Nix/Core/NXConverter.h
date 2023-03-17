#pragma once
#include "Header.h"

namespace NXConvert
{

//  wstring -> string 
std::string ws2s(const std::wstring& ws);

// string -> wstring
std::wstring s2ws(const std::string& s);

// string תСд
std::string s2lower(std::string s);
std::wstring s2lower(std::wstring s);

std::string GetPathOfImguiIni();

bool IsImageFileExtension(const std::string& strFilePath);

bool IsMaterialFileExtension(const std::string& strFilePath);

void getline_safe(std::ifstream& ifs, std::string& s);

bool IsMaterialDefaultPath(const std::string& s);

}