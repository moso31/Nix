#pragma once
#include "BaseDefs/Math.h"
#include "Ntr.h"
#include <string>

// ��¼һ�� nsl ���� ӳ�䵽 HLSL �Ժ�Ĵ�������
// ���Ӷ����е������У�
struct NXHLSLCodeRegion
{
	int firstRow = 0;
	int lastRow = 0;
};

extern const char* g_strCBufferGUIStyle[];
extern const int g_strCBufferGUIStyleCount;