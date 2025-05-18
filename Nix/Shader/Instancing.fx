#ifndef _INSTANCING_
#define _INSTANCING_

matrix GetInstancingMatrix(VS_INPUT vin)
{
	matrix mxInstance;
#ifdef GPU_INSTANCING
	mxInstance[0] = vin.row0;
	mxInstance[1] = vin.row1;
	mxInstance[2] = vin.row2;
	mxInstance[3] = vin.row3;
#else
	mxInstance[0] = float4(1.0f, 0.0f, 0.0f, 0.0f);
	mxInstance[1] = float4(0.0f, 1.0f, 0.0f, 0.0f);
	mxInstance[2] = float4(0.0f, 0.0f, 1.0f, 0.0f);
	mxInstance[3] = float4(0.0f, 0.0f, 0.0f, 1.0f);
#endif
	return mxInstance;
}

float4 GetInstancingPos(VS_INPUT vin)
{
#ifdef GPU_INSTANCING
	return vin.row3;
#else
	return float4(0.0f, 0.0f, 0.0f, 1.0f);
#endif
}

#endif // !_INSTANCING_