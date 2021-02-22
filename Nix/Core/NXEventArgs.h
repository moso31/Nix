#pragma once
#include "Header.h"

struct NXEventArgs
{
};

struct NXKeyEventArgs : public NXEventArgs
{
	USHORT VKey;
};

struct NXMouseEventArgs : public NXEventArgs
{
	USHORT X, Y;
	USHORT VMouse;
	USHORT VWheel;
	LONG LastX, LastY;
};
