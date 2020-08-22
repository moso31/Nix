#include "NXSphere.h"
#include "NXIntersection.h"
#include "SamplerMath.h"
#include "NXRandom.h"

using namespace SamplerMath;

NXSphere::NXSphere() :
	m_radius(0.0f),
	m_segmentVertical(0),
	m_segmentHorizontal(0)
{
}

void NXSphere::Init(float radius, int segmentHorizontal, int segmentVertical)
{
	int currVertIdx = 0;
	for (int i = 0; i < segmentVertical; i++)
	{
		float yDown = sinf(((float)i / (float)segmentVertical * 2.0f - 1.0f) * XM_PIDIV2) * radius;
		float yUp = sinf(((float)(i + 1) / (float)segmentVertical * 2.0f - 1.0f) * XM_PIDIV2) * radius;
		float radiusDown = sqrtf(radius * radius - yDown * yDown);
		float radiusUp = sqrtf(radius * radius - yUp * yUp);

		float yUVUp = Clamp(yUp * 0.5f + 0.5f, 0.0f, 1.0f);
		float yUVDown = Clamp(yDown * 0.5f + 0.5f, 0.0f, 1.0f);

		for (int j = 0; j < segmentHorizontal; j++)
		{
			float segNow = (float)j / (float)segmentHorizontal;
			float segNext = (float)(j + 1) / (float)segmentHorizontal;
			float angleNow = segNow * XM_2PI;
			float angleNext = segNext * XM_2PI;
			float xNow = cosf(angleNow);
			float zNow = sinf(angleNow);
			float xNext = cosf(angleNext);
			float zNext = sinf(angleNext);

			Vector3 pNowUp = { xNow * radiusUp, yUp, zNow * radiusUp };
			Vector3 pNextUp = { xNext * radiusUp, yUp, zNext * radiusUp };
			Vector3 pNowDown = { xNow * radiusDown, yDown, zNow * radiusDown };
			Vector3 pNextDown = { xNext * radiusDown, yDown, zNext * radiusDown };

			Vector2 uvNowUp = { segNow, yUVUp };
			Vector2 uvNextUp = { segNext, yUVUp };
			Vector2 uvNowDown = { segNow, yUVDown };
			Vector2 uvNextDown = { segNext, yUVDown };

			float invRadius = 1.0f / radius;
			Vector3 nNowUp, nNowDown, nNextUp, nNextDown;
			nNowUp = pNowUp * invRadius;
			nNextUp = pNextUp * invRadius;
			nNowDown = pNowDown * invRadius;
			nNextDown = pNextDown * invRadius;

			m_vertices.push_back({ pNowUp,		nNowUp,		uvNowUp });
			m_vertices.push_back({ pNextUp,		nNextUp,	uvNextUp });
			m_vertices.push_back({ pNextDown,	nNextDown,	uvNextDown });
			m_vertices.push_back({ pNowDown,	nNowDown,	uvNowDown });

			m_indices.push_back(currVertIdx);
			m_indices.push_back(currVertIdx + 1);
			m_indices.push_back(currVertIdx + 2);
			m_indices.push_back(currVertIdx);
			m_indices.push_back(currVertIdx + 2);
			m_indices.push_back(currVertIdx + 3);

			currVertIdx += 4;
		}
	}

	m_radius = radius;
	m_segmentVertical = segmentVertical;
	m_segmentHorizontal = segmentHorizontal;

	InitVertexIndexBuffer();
	InitAABB();
}

void NXSphere::UpdateSurfaceAreaInfo()
{
	m_fArea = XM_4PI * m_radius * m_radius;
}

float NXSphere::GetSurfaceArea()
{
	return m_fArea;
}

bool NXSphere::RayCast(const Ray& worldRay, NXHit& outHitInfo, float& outDist)
{
	Ray localRay = worldRay.Transform(m_worldMatrixInv);
	const Vector3& o = localRay.position;
	const Vector3& d = localRay.direction;

	float a = d.LengthSquared();
	float b = 2.0f * (o.x * d.x + o.y * d.y + o.z * d.z);
	float c = o.LengthSquared() - m_radius * m_radius;

	float dt2 = b * b - 4 * a * c;
	if (dt2 < 0.0f)
		return false;

	float dt = sqrtf(dt2);
	float inv2a = 0.5f / a;
	float t0 = (-b - dt) * inv2a;
	float t1 = (-b + dt) * inv2a;

	float tMin = min(t0, t1);
	float tMax = max(t0, t1);
	if (tMax < 0.0f)
		return false;

	// t should be always get the nearest positive value.
	float t = tMin > 0.0f ? tMin : tMax;

	Vector3 pHit = o + t * d;
	pHit *= m_radius / pHit.Length();

	// get theta and phi.
	float phi = atan2f(pHit.z, pHit.x);
	if (phi < 0.0f) phi += XM_2PI;
	float theta = acosf(Clamp(-pHit.y / m_radius, -1.0f, 1.0f));

	// get uvHit
	Vector2 uvHit(phi * XM_1DIV2PI, theta * XM_1DIVPI);

	Vector3 dpdu(-XM_2PI * pHit.z, 0.0f, XM_2PI * pHit.x);
	float piy = -XM_PI * pHit.y;
	Vector3 dpdv(piy * cosf(phi), XM_PI * m_radius * sinf(theta), piy * sinf(phi));
	dpdu.Normalize();
	dpdv.Normalize();

	auto pShape = std::dynamic_pointer_cast<NXSphere>(shared_from_this());
	outHitInfo = NXHit(pShape, pHit, uvHit, -localRay.direction, dpdu, dpdv);
	outDist = t;

	// ps: 二次曲面的计算是矢量级的，所以shading与Geometry相同。
	outHitInfo.SetShadingGeometry(dpdu, dpdv);
	return true;
}

void NXSphere::SampleForArea(Vector3& o_pos, Vector3& o_norm, float& o_pdfA)
{
	Vector2 vRandom = NXRandom::GetInstance()->CreateVector2();
	o_norm = UniformSampleSphere(vRandom);
	o_pos = o_norm * m_radius;
	o_pos = Vector3::Transform(o_pos, m_worldMatrix);
	o_norm = Vector3::TransformNormal(o_norm, m_worldMatrix);
	o_pdfA = 1.0f / GetSurfaceArea();
}

void NXSphere::SampleForSolidAngle(const NXHit& hitInfo, Vector3& o_pos, Vector3& o_norm, float& o_pdfW)
{
	float pdfA;
	SampleForArea(o_pos, o_norm, pdfA);
	Vector3 dirLight = hitInfo.position - o_pos;
	float cosTheta = o_norm.Dot(dirLight);
	float dist2 = dirLight.LengthSquared();
	if (dist2 < 0.0f)
		o_pdfW = 0.0f;
	else
	{
		dirLight.Normalize();
		o_pdfW = pdfA * dist2 / fabsf(o_norm.Dot(dirLight));
	}
}

float NXSphere::GetPdfSolidAngle(const NXHit& hitInfo, const Vector3& posLight, const Vector3& normLight, const Vector3& dirLight)
{
	float pdfA = GetPdfArea();
	float dist2 = Vector3::DistanceSquared(posLight, hitInfo.position);
	if (dist2 < 0.0f)
		return 0.0f;

	return pdfA * dist2 / fabsf(normLight.Dot(dirLight));
}
