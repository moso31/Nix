#define NodeDescUpdateIndicesNum 4
Texture2D txIn[NodeDescUpdateIndicesNum] : register(t0);
RWTexture2DArray<float> txOutAtlas : register(u0);

struct CBufferTerrainNodeDescUpdateInfo
{
	// 要替换的索引
    int newIndex;

	// 被替换的旧信息和大小
	// 注意如果是不需要replace，则size = 0;
    int2 replacePosWS;
    int replaceSize;
};

cbuffer cbUpdateIndices : register(b0)
{
    CBufferTerrainNodeDescUpdateInfo m_updateIndices[NodeDescUpdateIndicesNum];
}

[numthreads(8, 8, 1)]
void CS(uint3 dtid : SV_DispatchThreadID)
{
    uint2 pixelPos = dtid.xy;
    uint inputIndex = dtid.z;
    
    float height = txIn[inputIndex].Load(int3(pixelPos, 0)).r;
    txOutAtlas[uint3(pixelPos, m_updateIndices[inputIndex].newIndex)] = height;
}
