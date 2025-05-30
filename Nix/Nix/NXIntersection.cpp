#include "NXIntersection.h"
#include "NXScene.h"
#include "NXRenderableObject.h"
#include "NXSubMesh.h"

NXHit::NXHit(NXSubMeshBase* pSubMesh, const Vector3& position, const Vector2& uv, const Vector3& direction, const Vector3& dpdu, const Vector3& dpdv) :
	pSubMesh(pSubMesh),
	position(position),
	uv(uv),
	direction(direction),
	dpdu(dpdu),
	dpdv(dpdv),
	normal(dpdv.Cross(dpdu)),
	faceIndex(-1)
{
	normal.Normalize();
}

void NXHit::LocalToWorld()
{
	Matrix mxWorld = pSubMesh->GetRenderableObject()->GetWorldMatrix();
	position = Vector3::Transform(position, mxWorld);
	Vector3::TransformNormal(direction, mxWorld).Normalize(direction);
	Vector3::TransformNormal(normal, mxWorld).Normalize(normal);
	Vector3::TransformNormal(dpdu, mxWorld).Normalize(dpdu);
	Vector3::TransformNormal(dpdv, mxWorld).Normalize(dpdv);
}

void NXHit::Reset()
{
	pSubMesh = nullptr;
}
