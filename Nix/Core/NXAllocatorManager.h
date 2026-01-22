#pragma once
#include "BaseDefs/NixCore.h"
#include "NXInstance.h"
#include "NXShaderVisibleDescriptorHeap.h"
#include "NXTextureLoader.h"
#include "NXNullDescriptor.h"

// 资源
#define NXAllocator_CB		NXAllocatorManager::GetInstance()->GetCBAllocator()			// cb
#define NXAllocator_SB		NXAllocatorManager::GetInstance()->GetSBAllocator()			// vb, ib, instance-data
#define NXAllocator_RB		NXAllocatorManager::GetInstance()->GetRBAllocator()			// readback buffer
#define NXAllocator_RWB		NXAllocatorManager::GetInstance()->GetRWBAllocator()		// rw buffer
#define NXAllocator_Tex		NXAllocatorManager::GetInstance()->GetTextureAllocator()	// texture，但其中目前仅文件、高度图、cpu程序生成这几种。但不包括RT（RT单独）

// 描述符
#define NXAllocator_SRV		NXAllocatorManager::GetInstance()->GetSRVAllocator()
#define NXAllocator_RTV		NXAllocatorManager::GetInstance()->GetRTVAllocator()
#define NXAllocator_DSV		NXAllocatorManager::GetInstance()->GetDSVAllocator()
#define NXAllocator_NULL 	NXAllocatorManager::GetInstance()->GetNullDescriptorAllocator()
#define NXShVisDescHeap		NXAllocatorManager::GetInstance()->GetShaderVisibleDescriptorAllocator()

// 上传系统
#define NXUploadSys			NXAllocatorManager::GetInstance()->GetNXUploadSystem()
#define NXReadbackSys		NXAllocatorManager::GetInstance()->GetNXReadbackSystem()

// 纹理加载器
#define NXTexLoader			NXAllocatorManager::GetInstance()->GetTextureLoader()

using namespace ccmem;

class NXAllocatorManager : public NXInstance<NXAllocatorManager>
{
public:
	NXAllocatorManager();
	virtual ~NXAllocatorManager();

	void Init();

	CommittedBufferAllocator*		GetCBAllocator()					{ return m_pCBAllocator.get(); }
	CommittedBufferAllocator*		GetSBAllocator()					{ return m_pSBAllocator.get(); }
	CommittedBufferAllocator*		GetRBAllocator()					{ return m_pRBAllocator.get(); }
	PlacedBufferAllocator*			GetRWBAllocator()					{ return m_pRWBAllocator.get(); }
	PlacedBufferAllocator*			GetTextureAllocator()				{ return m_pTextureAllocator.get(); }

	DescriptorAllocator<false>*		GetSRVAllocator()					{ return m_pSRVAllocator.get(); }
	DescriptorAllocator<false>*		GetRTVAllocator()					{ return m_pRTVAllocator.get(); }
	DescriptorAllocator<false>*		GetDSVAllocator()					{ return m_pDSVAllocator.get(); }
	NXNullDescriptor*				GetNullDescriptorAllocator()		{ return m_pNullDescriptorAllocator.get(); }

	NXUploadSystem*					GetNXUploadSystem()					{ return m_pUpdateSystem.get(); }
	NXReadbackSystem*				GetNXReadbackSystem()				{ return m_pReadbackSystem.get(); }
	NXTextureLoader*				GetTextureLoader()					{ return m_pTextureLoader.get(); }

	DescriptorAllocator<true>*		GetShaderVisibleDescriptorAllocator()	{ return m_pShaderVisibleDescAllocator.get(); }

	void Release();

private:
	std::unique_ptr<CommittedBufferAllocator>		m_pCBAllocator;	
	std::unique_ptr<CommittedBufferAllocator>		m_pSBAllocator;
	std::unique_ptr<CommittedBufferAllocator>		m_pRBAllocator;
	std::unique_ptr<PlacedBufferAllocator>			m_pRWBAllocator;
	std::unique_ptr<PlacedBufferAllocator>			m_pTextureAllocator;

	std::unique_ptr<DescriptorAllocator<false>>		m_pSRVAllocator;
	std::unique_ptr<DescriptorAllocator<false>>		m_pRTVAllocator;
	std::unique_ptr<DescriptorAllocator<false>>		m_pDSVAllocator;
	std::unique_ptr<NXNullDescriptor>				m_pNullDescriptorAllocator;

	std::unique_ptr<NXUploadSystem>					m_pUpdateSystem;
	std::unique_ptr<NXReadbackSystem>				m_pReadbackSystem;
	std::unique_ptr<NXTextureLoader>				m_pTextureLoader;

	// shader-visible descriptor allocator
	std::unique_ptr<DescriptorAllocator<true>> 	m_pShaderVisibleDescAllocator;

	// 以上所有都使用独立线程运行，使用一个线程容器管理它们
	std::vector<std::thread> m_threads;
	std::atomic<bool> m_bRunning;
};
