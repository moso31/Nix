#pragma once
#include "BaseDefs/Math.h"
#include "BaseDefs/NixCore.h"
#include "BaseDefs/CppSTLFully.h"

struct NXTerrainStreamingLoadTask
{
	Int2 terrainID; // 要加载的地形块ID
	Int2 relativePos; // 要加载的地形node的相对位置
	Int2 size; // 要加载的地形node的尺寸
};

class NXTerrainStreamingAsyncLoader
{
public:
	NXTerrainStreamingAsyncLoader() {};
	~NXTerrainStreamingAsyncLoader() {};

	void AddTask();
	void Update();
	
private:

};