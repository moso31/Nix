#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include <iterator>

namespace NXConvert
{
	//  wstring -> string 
	std::string ws2s(const std::wstring& ws);

	// string -> wstring
	std::wstring s2ws(const std::string& s);

	std::string s2lower(const std::string& s);
	std::wstring s2lower(const std::wstring& s);

	// 去掉 str 中的所有空格和tab
	std::string Trim(const std::string& str);

	// 分割字符串
	std::vector<std::string> split(const std::string& str, const std::string& delimiters = " \t");
}

