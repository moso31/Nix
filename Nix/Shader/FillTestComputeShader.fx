ConsumeStructuredBuffer<uint2> m_inBuffer : register(u0);
AppendStructuredBuffer<uint2> m_outBuffer : register(u1);
AppendStructuredBuffer<uint3> m_final : register(u2);

cbuffer cbTerrainParam : register(b0)
{
    float3 m_camPos;
    float m_scale;
    uint m_lod;
    float m_currLodDist; // ��ǰlod�ȼ��ľ���
}

[numthreads(1, 1, 1)]
void CS_Pass(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 dtid = dispatchThreadID.xy;
    
    // ��һ������Lod
    uint2 c = m_inBuffer.Consume();

    // �����ĸ��ӿ鵽��ǰlod�ľ���
    float3 childCenterPos0 = float3(c.x, 0, c.y) * m_scale + float3(m_scale * 0.25, 0, m_scale * 0.25);
    float3 childCenterPos1 = float3(c.x, 0, c.y) * m_scale + float3(m_scale * 0.25, 0, m_scale * 0.75);
    float3 childCenterPos2 = float3(c.x, 0, c.y) * m_scale + float3(m_scale * 0.75, 0, m_scale * 0.25);
    float3 childCenterPos3 = float3(c.x, 0, c.y) * m_scale + float3(m_scale * 0.75, 0, m_scale * 0.75);
    float dist0 = length(childCenterPos0 - m_camPos);
    float dist1 = length(childCenterPos1 - m_camPos);
    float dist2 = length(childCenterPos2 - m_camPos);
    float dist3 = length(childCenterPos3 - m_camPos);

    if (dist0 < m_currLodDist && dist1 < m_currLodDist && dist2 < m_currLodDist && dist3 < m_currLodDist)
    {
        // ����ĸ��ӿ鶼�ڵ�ǰLod�ľ����ڣ���ֱ���������Lod
		m_outBuffer.Append(c * 2 + uint2(0, 0));
		m_outBuffer.Append(c * 2 + uint2(0, 1));
		m_outBuffer.Append(c * 2 + uint2(1, 0));
		m_outBuffer.Append(c * 2 + uint2(1, 1));
    }
    else
    {
        m_final.Append(uint3(c, m_lod));
    }
}
