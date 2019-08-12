#pragma once
#include "Header.h"

class DirectResources
{
public:
	HRESULT InitDevice();
	void	ClearDevices();

	Vector2 GetViewPortSize();

private:
	D3D11_VIEWPORT	m_ViewPort;
};