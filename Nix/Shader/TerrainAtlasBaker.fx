#define LoadTexGroupNum 4
Texture2D txIn[LoadTexGroupNum] : register(t0);
RWTexture2DArray<float> txOutAtlas : register(u0);

cbuffer cbUpdateIndex : register(b0)
{
    // 注意：在c++层这玩意是个std::vector<int>(4)
    int4 updateIndices; 
}

[numthreads(8, 8, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{
    uint2 pixelPos = dtid.xy;
    uint inputIndex = dtid.z;
    
    float height = txIn[inputIndex].Load(int3(pixelPos, 0)).r;
    txOutAtlas[uint3(pixelPos, updateIndices[inputIndex])] = height;
}
