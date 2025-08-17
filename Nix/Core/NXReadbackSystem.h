#pragma once
#include "XAllocCommon.h"
#include "BaseDefs/DX12.h"

class NXReadbackRingBuffer;
struct NXReadbackTask
{
    NXReadbackTask() {}

    void Reset()
    {
        ringPos = 0;
        byteSize = 0;
        pCallback = nullptr;
        mainRenderFenceValue = UINT64_MAX;
    }

    // ������ɺ�Ļص�
    std::function<void()> pCallback = nullptr;

    // ��RingBuffer�е�λ�úʹ�С
    uint32_t ringPos = 0;
    uint32_t byteSize = 0;

    uint64_t mainRenderFenceValue = UINT64_MAX;
};

struct NXReadbackContext
{
    NXReadbackContext(const std::string& name) : name(name) {}

    NXReadbackTask* pOwner = nullptr;

    // ringBuffer����ʱ��Դ���塢��ʱ��Դ�ϴ���ӳ�䡢��ʱ��Դ�ϴ���ƫ����
    ID3D12Resource* pResource = nullptr;
    uint8_t* pResourceData = nullptr;
    uint32_t pResourceOffset = 0;
    std::string name;
};

class NXReadbackRingBuffer
{
public:
    NXReadbackRingBuffer(ID3D12Device* pDevice, uint32_t bufferSize);
    ~NXReadbackRingBuffer();

    bool CanAlloc(uint32_t byteSize);
    bool Build(uint32_t byteSize, NXReadbackTask& oTask);
    void Finish(const NXReadbackTask& task);

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

class NXReadbackSystem
{
    const static uint32_t TASK_NUM = 16;

public:
    NXReadbackSystem(ID3D12Device* pDevice);
    ~NXReadbackSystem();

    bool BuildTask(int byteSize, NXReadbackContext& taskResult);
    void FinishTask(const NXReadbackContext& result, const std::function<void()>& pCallBack = nullptr);
    void Update();
    void UpdatePendingTaskFenceValue(uint64_t mainRenderFenceValue);

private:
    ID3D12Device* m_pDevice;

    NXReadbackTask m_tasks[TASK_NUM];

    uint32_t m_taskStart = 0;
    uint32_t m_taskUsed = 0;

    NXReadbackRingBuffer m_ringBuffer;

    // �ض�task����֡ĩ��FenceValue
    // �������ض�taskʱ����ǰ֡��CPUָ�û���꣬ʵ�ʲ�֪��֡ĩ��FenceValue
    // ��������pendingTask��һ�»ض�taskָ�룬��֡ĩFenceValue����ʱ ͬ��ˢ�������ֵ
    std::vector<NXReadbackTask*> m_pendingTask;

    // ������������ǱȽϼ򵥴ֱ��ģ�ÿ����������������Щ�����Ŀ���������
    // �ϴ�ϵͳ�Ĵ�ͷ������BeginTask()������FinishTask()��ʼǰ���ʱ��ĸ��ֲ����ϣ�����Щ�����Ǳ�¶���ϲ㣬������߳�ͬʱ���õġ�
    std::mutex m_mutex;
    std::condition_variable m_condition;
};
