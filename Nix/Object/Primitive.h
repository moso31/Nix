#pragma once
#include "ShaderStructures.h"

class Box;
class Sphere;
class Cylinder;
class Cone;

class Primitive
{
public:
	Primitive();
	~Primitive();

	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void Release() = 0;

	void SetMaterial(const shared_ptr<Material>& material);

protected:
	ConstantBufferMaterial	m_cbDataMaterial;
	ID3D11Buffer*			m_cbMaterial;
	shared_ptr<Material>	m_pMaterial;
};
