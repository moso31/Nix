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

    static void Write(const char* format, ...);
};
