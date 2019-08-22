#pragma once
#include <Windows.h>

namespace NX
{
	void ThrowIfFailed(HRESULT hr);
	void MessageBoxIfFailed(HRESULT hr, LPCWSTR errMsg);
}