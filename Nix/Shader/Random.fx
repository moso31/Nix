// α����ݶ���������Χ[0, 1)
// http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
float InterleavedGradientNoise(float2 value, int offset)
{
	const float3 magic = float3(0.06711056f * 100, 0.00583715f * 100, 52.9829189f);
	float2 scale = float2(1.114514f, 2.1919810f);	// scale��Ҫ��const����Ȼoffset=0��ʱ��������Ż�����
	value += offset * scale;
	return frac(magic.z * frac(dot(value, magic.xy)));
}
