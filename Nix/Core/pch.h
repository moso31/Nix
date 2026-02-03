// pch.h - 预编译头文件
// 此文件包含频繁使用但不常修改的标准系统头文件和项目头文件
// 启用 PCH 可显著改善 IntelliSense 响应速度和编译性能

#pragma once

// ===== Windows SDK 核心 =====
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <shellapi.h>  // WIN32_LEAN_AND_MEAN 会排除此头文件，需手动包含（用于拖放文件功能）

// 取消 Windows 宏污染（避免与 rapidjson 等库冲突）
#ifdef GetObject
#undef GetObject
#endif

// ===== C++ 标准库 =====
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <functional>
#include <algorithm>
#include <cstdint>
#include <atomic>

// ===== DirectX 12 =====
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXColors.h>
#include <dxcapi.h>

#ifdef DEBUG
#include <pix3.h>
#endif

using namespace DirectX;
using namespace Microsoft::WRL;

// ===== Nix 基础定义 =====
#include "BaseDefs/NixCore.h"
#include "BaseDefs/DX12.h"
#include "BaseDefs/Math.h"
#include "BaseDefs/DearImGui.h"
#include "Ntr.h"
#include "NXGlobalDefinitions.h"
