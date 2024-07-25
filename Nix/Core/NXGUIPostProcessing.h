#pragma once

class NXColorMappingRenderer;
class NXGUIPostProcessing
{
public:
	NXGUIPostProcessing(NXColorMappingRenderer* pPostProcessingRenderer = nullptr);
	virtual ~NXGUIPostProcessing() {}

	void Render();

private:
	// 2023.3.10 Ŀǰ PostProcessing ֻ�� ColorMapping���� ������������ʱ����ͬ��ʡ�
	NXColorMappingRenderer* m_pPostProcessingRenderer;
};