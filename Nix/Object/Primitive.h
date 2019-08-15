#pragma once
#include "ShaderStructures.h"

class Primitive
{
public:
	Primitive();
	~Primitive();

	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void Release() = 0;

private:

};
