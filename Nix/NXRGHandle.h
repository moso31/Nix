#pragma once
#include "BaseDefs/NixCore.h"

class NXRGHandle
{
	static int s_id;
	static std::map<int, int> s_indexToVersion;

public:
	// �½���Դ��index����
	NXRGHandle() : 
		m_index(s_id++)
	{
		s_indexToVersion[m_index] = 0;
		m_version = s_indexToVersion[m_index];
	}

	// ������Դʱ���ã�index�����ӣ�version����
	NXRGHandle(NXRGHandle* handle) 
	{
		m_index = handle->m_index;
		m_version = s_indexToVersion[m_index];
	}

	static void Reset()
	{
		s_id = 0;
		s_indexToVersion.clear();
	}

private:
	uint16_t m_index;
	uint16_t m_version;
};
