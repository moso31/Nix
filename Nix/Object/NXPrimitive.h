#pragma once
#include "ShaderStructures.h"

class NXBox;
class NXSphere;
class NXCylinder;
class NXCone;
class NXPlane;

class NXPrimitive
{
public:
	NXPrimitive();
	~NXPrimitive();

	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void Release() = 0;

	void SetMaterial(const shared_ptr<NXMaterial>& material);

protected:
	ConstantBufferMaterial	m_cbDataMaterial;
	ID3D11Buffer*			m_cbMaterial;
	shared_ptr<NXMaterial>	m_pMaterial;
};
