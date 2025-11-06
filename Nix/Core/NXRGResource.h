#pragma once
#include "NXRGUtil.h"

struct NXRGDescription
{
};

class NXRGResource
{
public:
	// 用于创建老资源的新版本
	NXRGResource(NXRGResource* pOldResource) :
		m_name(pOldResource->m_name),
		m_description(pOldResource->m_description),
		m_handle(pOldResource->m_handle.index, pOldResource->m_handle.version + 1)
	{
		NXRGHandle::s_maxVersions[m_handle.index] = m_handle.version; // 上面已经+1了
	}

	// 用于创建新资源
	NXRGResource(const std::string& name, const NXRGDescription& description) :
		m_name(name),
		m_description(description),
		m_handle(NXRGHandle::s_nextIndex++)
	{
		NXRGHandle::s_maxVersions[m_handle.index] = m_handle.version; // 初始=0
	}

	~NXRGResource() {}

	const std::string& GetName() { return m_name; }
	const NXRGDescription& GetDescription() { return m_description; }
	NXRGHandle GetHandle() { return m_handle; }

private:
	std::string m_name;
	NXRGDescription m_description;
	NXRGHandle m_handle;
};

