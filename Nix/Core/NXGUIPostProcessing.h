#pragma once
#include "Header.h"

class NXColorMappingRenderer;

class NXGUIPostProcessing
{
public:
	NXGUIPostProcessing(NXColorMappingRenderer* pPostProcessingRenderer = nullptr);
	~NXGUIPostProcessing() {}

	void Render();

private:
	// 2023.3.10 目前 PostProcessing 只有 ColorMapping…… 所以这两个暂时算是同义词。
	NXColorMappingRenderer* m_pPostProcessingRenderer;
};