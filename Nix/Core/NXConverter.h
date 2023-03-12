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

std::string GetPathOfImguiIni();

}