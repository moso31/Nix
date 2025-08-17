#pragma once
#include "XAllocCommon.h"
#include "BaseDefs/DX12.h"

class NXUploadRingBuffer;
struct NXUploadTask
{
    NXUploadTask();

    void Reset()
    {
		ringPos = 0;
		byteSize = 0;
		fenceValue = 0;
	}

	ID3D12GraphicsCommandList* pCmdList = nullptr;
	ID3D12CommandAllocator* pCmdAllocator = nullptr;

	// ��¼ task ��fenceValue
	// ���� BuildTask()ʱ��fenceValue = -1����ʾδ��ɣ�
	// ���ʱ FinishTask() ������ˢ�����ֵΪһ��������ֵ��
	// Ȼ��ÿ֡ Update() ʱ��GPU���ܽ����Ƴ�
	uint64_t fenceValue = 0;

    // ������ɺ�Ļص�
    std::function<void()> pCallback = nullptr;

	// ��RingBuffer�е�λ�úʹ�С
	uint32_t ringPos = 0;
	uint32_t byteSize = 0;

    NXUploadRingBuffer* pRingBuffer = nullptr;
};

struct NXUploadContext
{
    NXUploadContext(const std::string& name) : name(name) {}

    NXUploadTask* pOwner = nullptr;

    // ringBuffer����ʱ��Դ���塢��ʱ��Դ�ϴ���ӳ�䡢��ʱ��Դ�ϴ���ƫ����
	ID3D12Resource* pResource = nullptr;
	uint8_t* pResourceData = nullptr;
    uint32_t pResourceOffset = 0;
    std::string name;
};

class NXUploadRingBuffer
{
public:
    NXUploadRingBuffer(ID3D12Device* pDevice, uint32_t bufferSize);
    ~NXUploadRingBuffer();

    bool CanAlloc(uint32_t byteSize);
    bool Build(uint32_t byteSize, NXUploadTask& oTask);
    void Finish(const NXUploadTask& task);

    ID3D12Resource* GetResource() { return m_pResource; }
    uint8_t* GetResourceMappedData() { return m_pResourceData; }

private:
    uint32_t m_size;

    // ��¼ring�е��ѷ��䷶Χ
    uint32_t m_usedStart;
    uint32_t m_usedEnd;

    ID3D12Device* m_pDevice;

    ID3D12Resource* m_pResource;
    uint8_t* m_pResourceData;
};

class NXUploadSystem
{
    const static uint32_t TASK_NUM = 16;

public:
    NXUploadSystem(ID3D12Device* pDevice);
    ~NXUploadSystem();

    bool BuildTask(int byteSize, NXUploadContext& taskResult);
    void FinishTask(const NXUploadContext& result, const std::function<void()>& pCallBack = nullptr);
    void Update();
    ID3D12Fence* GetFence() { return m_pFence; }
    const uint64_t GetCurrentFenceValue() { return m_frameFenceValue.Current(); }

private:
    ID3D12Device* m_pDevice;
    ID3D12CommandQueue* m_pCmdQueue;
    ID3D12Fence* m_pFence;
    uint64_t m_fenceValue = 0;
    MultiFrame<uint64_t> m_frameFenceValue;

    ID3D12CommandQueue* m_pSyncCmdQueue;

    NXUploadTask m_tasks[TASK_NUM];

    uint32_t m_taskStart = 0;
    uint32_t m_taskUsed = 0;

    NXUploadRingBuffer m_ringBuffer;

    // ������������ǱȽϼ򵥴ֱ��ģ�ÿ����������������Щ�����Ŀ���������
    // �ϴ�ϵͳ�Ĵ�ͷ������BeginTask()������FinishTask()��ʼǰ���ʱ��ĸ��ֲ����ϣ�����Щ�����Ǳ�¶���ϲ㣬������߳�ͬʱ���õġ�
    std::mutex m_mutex;
    std::condition_variable m_condition;
};
