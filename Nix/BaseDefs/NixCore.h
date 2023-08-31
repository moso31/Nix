#pragma once

#include <windows.h>

#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#define SafeDeleteArray(x) { delete[] x; x = nullptr; }
#define SafeDelete(x) { delete x; x = nullptr; }
#define SafeReleaseCOM(x) { if (x) { x->Release(); x = nullptr; } }
#define SafeRelease(x) { if (x) { x->Release(); SafeDelete(x); } }

#include <string>
const std::string   g_str_empty = "";

// texture file paths
const std::string	g_defaultTex_white_str = ".\\Resource\\white1x1.png";
const std::string	g_defaultTex_normal_str = ".\\Resource\\normal1x1.png";
const std::wstring	g_defaultTex_white_wstr = L".\\Resource\\white1x1.png";
const std::wstring	g_defaultTex_normal_wstr = L".\\Resource\\normal1x1.png";

// material templates
const std::string   g_material_template_standardPBR = ".\\Resource\\shaders\\Default.nsl";
