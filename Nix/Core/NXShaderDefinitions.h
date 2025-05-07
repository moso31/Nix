#pragma once
#include "BaseDefs/Math.h"
#include "Ntr.h"
#include <string>

// 记录一段 nsl 函数 映射到 HLSL 以后的代码坐标
// （从多少行到多少行）
struct NXHLSLCodeRegion
{
	int firstRow = 0;
	int lastRow = 0;
};

extern const char* g_strCBufferGUIStyle[];
extern const int g_strCBufferGUIStyleCount;