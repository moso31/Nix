ConsumeStructuredBuffer<int2> m_inBuffer : register(u0);
AppendStructuredBuffer<int2> m_outBuffer : register(u1);
AppendStructuredBuffer<int3> m_final : register(u2);

cbuffer cbTerrainParam : register(b0)
{
    float3 m_camPos;
    float m_scale;
    uint m_lod;
    float m_currLodDist; // 当前lod等级的距离
}

[numthreads(1, 1, 1)]
void CS_Pass(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 dtid = dispatchThreadID.xy;
    
    // 拿一个父级Lod
    int2 c = m_inBuffer.Consume();

    // 计算四个子块到当前lod的距离
    float3 childCenterPos0 = float3(c.x, 0, c.y) * m_scale + float3(m_scale * 0.25, 0, m_scale * 0.25);
    float3 childCenterPos1 = float3(c.x, 0, c.y) * m_scale + float3(m_scale * 0.25, 0, m_scale * 0.75);
    float3 childCenterPos2 = float3(c.x, 0, c.y) * m_scale + float3(m_scale * 0.75, 0, m_scale * 0.25);
    float3 childCenterPos3 = float3(c.x, 0, c.y) * m_scale + float3(m_scale * 0.75, 0, m_scale * 0.75);
    float dist0 = length(childCenterPos0 - m_camPos);
    float dist1 = length(childCenterPos1 - m_camPos);
    float dist2 = length(childCenterPos2 - m_camPos);
    float dist3 = length(childCenterPos3 - m_camPos);

    if (m_lod < 5 && dist0 < m_currLodDist && dist1 < m_currLodDist && dist2 < m_currLodDist && dist3 < m_currLodDist)
    {
        // 如果四个子块都在当前Lod的距离内，输出到下一轮input，继续细分
		m_outBuffer.Append(c * 2 + int2(0, 0));
		m_outBuffer.Append(c * 2 + int2(0, 1));
		m_outBuffer.Append(c * 2 + int2(1, 0));
		m_outBuffer.Append(c * 2 + int2(1, 1));
    }
    else
    {
        m_final.Append(int3(c, m_lod));
    }
}
