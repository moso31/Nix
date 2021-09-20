#include "NXPBRLight.h"
#include "SamplerMath.h"
#include "NXScene.h"
#include "NXCubeMap.h"
#include "NXPrimitive.h"
#include "NXRandom.h"

using namespace SamplerMath;

ConstantBufferPointLight NXPBRPointLight::GetConstantBuffer()
{
	ConstantBufferPointLight cb;
	cb.position = Position;
	cb._0 = 0;
	cb.color = Intensity;
	cb._1 = 0;
	return cb;
}

NXPBRDistantLight::NXPBRDistantLight(const Vector3& Direction, const Vector3& Radiance, Vector3 WorldCenter, float WorldRadius) :
	Direction(Direction), 
	Radiance(Radiance),
	WorldCenter(WorldCenter),
	WorldRadius(WorldRadius)
{
	this->Direction.Normalize();
}

ConstantBufferDistantLight NXPBRDistantLight::GetConstantBuffer()
{
	ConstantBufferDistantLight cb;
	cb.direction = Direction;
	cb.color = Radiance;
	return cb;
}

NXPBREnvironmentLight::NXPBREnvironmentLight(NXCubeMap* pCubeMap, const Vector3& Radiance, Vector3 WorldCenter, float WorldRadius) :
	m_pCubeMap(pCubeMap),
	Radiance(Radiance),
	WorldCenter(WorldCenter),
	WorldRadius(WorldRadius)
{
}