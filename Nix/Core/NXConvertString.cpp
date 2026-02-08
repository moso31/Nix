#include "NXConvertString.h"

namespace NXConvert
{
	std::string ws2s(const std::wstring& ws)
	{
		std::string s;
		s.resize(ws.length());
		if (ws.empty()) return s;

		size_t size;
		wcstombs_s(&size, &s[0], s.size() + 1, ws.c_str(), ws.size());
		return s;
	}

	std::wstring s2ws(const std::string& s)
	{
		return std::wstring(s.begin(), s.end());
	}

	std::string s2lower(const std::string& s)
	{
		std::string result;
		std::transform(s.begin(), s.end(), std::back_inserter(result), ::tolower);
		return result;
	}

	std::wstring s2lower(const std::wstring& s)
	{
		std::wstring result;
		std::transform(s.begin(), s.end(), std::back_inserter(result), ::towlower);
		return result;
	}

	std::string Trim(const std::string& str)
	{
		// 去掉 str 中的所有空格和tab
		const auto begin = str.find_first_not_of(" \t");
		if (begin == std::string::npos) return "";
		const auto end = str.find_last_not_of(" \t");
		return str.substr(begin, end - begin + 1);
	}

	std::vector<std::string> split(const std::string& str, const std::string& delimiters)
	{
		std::vector<std::string> tokens;
		size_t start = str.find_first_not_of(delimiters), end = 0;

		while (start != std::string::npos)
		{
			end = str.find_first_of(delimiters, start);
			tokens.emplace_back(str.substr(start, end - start));
			start = str.find_first_not_of(delimiters, end);
		}
		return tokens;
	}
}
