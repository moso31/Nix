#pragma once
#include "NXDXHelper.h"

namespace NX
{
	void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
			throw(hr);
	}

	void MessageBoxIfFailed(HRESULT hr, LPCWSTR errMsg)
	{
		if (FAILED(hr))
		{
			MessageBox(nullptr, errMsg, L"error", MB_OK);
			throw(hr);
		}
	}
}