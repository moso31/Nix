#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <cstdarg>
#include <Windows.h>
#include <DbgHelp.h>

#pragma comment(lib, "DbgHelp.lib")

class NXLog 
{
public:
    static void Init();
    static void Log(const char* format, ...);
    static void LogWithStackTrace(const char* format, ...);
    static void Release();

private:
    static std::filesystem::path filePath;
    static std::ofstream logFile;
};
