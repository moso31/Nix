struct AABB
{
	float4 Center; // ÖÐÐÄµã
	float4 Extents; // °ë¾¶
};

RWStructuredBuffer<AABB> gOutput : register(u0);

[numthreads(64, 1, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;
    gOutput[index] = gOutput[index];
}
