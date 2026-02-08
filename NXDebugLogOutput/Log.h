#pragma once
#include <thread>
#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <sstream>

class NXPrint
{
public:
    static void Init();

    static bool WriteCondition(const int id);

    static void Write(const int id, const char* format, ...);
};
