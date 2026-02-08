#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <DirectXColors.h>
#include <d3dcompiler.h>

#include <set>
#include <functional>
#include <algorithm>
#include <vector>
#include <list>
#include <string>
#include <mutex>
#include <cassert>
#include <iostream>
#include <atomic>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <condition_variable>

#include "Log.h"

namespace ccmem
{
	// 全局ID生成器
	uint64_t GenerateUniqueTaskID();
}
