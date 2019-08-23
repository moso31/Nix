#include "NXSphere.h"
#include "WICTextureLoader.h"

void NXSphere::Init(float radius, int segmentHorizontal, int segmentVertical)
{
	int currVertIdx = 0;
	for (int i = 0; i < segmentVertical; i++)
	{
		float yDown = ((float)i / (float)segmentVertical * 2.0f - 1.0f) * radius;
		float yUp = ((float)(i + 1) / (float)segmentVertical * 2.0f - 1.0f) * radius;
		float radiusDown = sqrtf(radius * radius - yDown * yDown);
		float radiusUp = sqrtf(radius * radius - yUp * yUp);

		for (int j = 0; j < segmentHorizontal; j++)
		{
			float segNow = (float)j / (float)segmentHorizontal;
			float segNext = (float)(j + 1) / (float)segmentHorizontal;
			float angleNow = segNow * XM_2PI;
			float angleNext = segNext * XM_2PI;
			float xNow = sinf(angleNow);
			float zNow = cosf(angleNow);
			float xNext = sinf(angleNext);
			float zNext = cosf(angleNext);

			Vector3 pNowUp = { xNow * radiusUp, yUp, zNow * radiusUp };
			Vector3 pNextUp = { xNext * radiusUp, yUp, zNext * radiusUp };
			Vector3 pNowDown = { xNow * radiusDown, yDown, zNow * radiusDown };
			Vector3 pNextDown = { xNext * radiusDown, yDown, zNext * radiusDown };

			Vector2 uvNowUp = { segNow, yUp };
			Vector2 uvNextUp = { segNext, yUp };
			Vector2 uvNowDown = { segNow, yDown };
			Vector2 uvNextDown = { segNext, yDown };

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
			m_indices.push_back(currVertIdx + 2);
			m_indices.push_back(currVertIdx + 1);
			m_indices.push_back(currVertIdx);
			m_indices.push_back(currVertIdx + 3);
			m_indices.push_back(currVertIdx + 2);

			currVertIdx += 4;
		}
	}

	m_radius = radius;
	m_segmentVertical = segmentVertical;
	m_segmentHorizontal = segmentHorizontal;

	InitVertexIndexBuffer();
}

void NXSphere::Render()
{
	g_pContext->VSSetConstantBuffers(0, 1, &m_pConstantBuffer);
	g_pContext->PSSetShaderResources(0, 1, &m_pTextureSRV);
	g_pContext->DrawIndexed((UINT)m_indices.size(), 0, 0);
}

void NXSphere::Release()
{
	if (m_pVertexBuffer)	m_pVertexBuffer->Release();
	if (m_pIndexBuffer)		m_pIndexBuffer->Release();
	if (m_pConstantBuffer)	m_pConstantBuffer->Release();
	if (m_pTextureSRV)		m_pTextureSRV->Release();
}
