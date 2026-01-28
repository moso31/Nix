#pragma once
#include "XAllocCommon.h"
#include "BaseDefs/DX12.h"
#include "Ntr.h"

enum class NXReadbackType 
{ 
    Unknown,
    Buffer, 
    Texture 
};

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

    // 任务完成后的回调
    std::function<void()> pCallback = nullptr;

    // 在RingBuffer中的位置和大小
    uint32_t ringPos = 0;
    uint32_t byteSize = 0;

    uint64_t mainRenderFenceValue = UINT64_MAX;
};

struct NXReadbackContext
{
    NXReadbackContext(const std::string& name) : name(name) {}

    NXReadbackTask* pOwner = nullptr;

    // ringBuffer的临时资源本体、临时资源上传堆映射、临时资源上传堆偏移量
    ID3D12Resource* pResource = nullptr;
    uint8_t* pResourceData = nullptr;
    uint32_t pResourceOffset = 0;
    std::string name;
    
    NXReadbackType type = NXReadbackType::Unknown; 

    // 回读纹理时专用
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    uint32_t numRows = 0;
    uint64_t rowSizeInBytes = 0;
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

    // 记录ring中的已分配范围
    uint32_t m_usedStart;
    uint32_t m_usedEnd;

    ID3D12Device* m_pDevice;

    ID3D12Resource* m_pResource;
    uint8_t* m_pResourceData;
};

class NXTexture;
class NXReadbackSystem
{
    const static uint32_t TASK_NUM = 16;

public:
    NXReadbackSystem(ID3D12Device* pDevice);
    ~NXReadbackSystem();

    // 回读Buffer
    bool BuildTask(int byteSize, NXReadbackContext& taskResult);
    // 回读Texture
    bool BuildTask(const Ntr<NXTexture>& pTexture, NXReadbackContext& taskResult);

    void FinishTask(const NXReadbackContext& result, const std::function<void()>& pCallBack = nullptr);
    void Update();
    void UpdatePendingTaskFenceValue(uint64_t mainRenderFenceValue);

private:
    ID3D12Device* m_pDevice;

    NXReadbackTask m_tasks[TASK_NUM];

    uint32_t m_taskStart = 0;
    uint32_t m_taskUsed = 0;

    NXReadbackRingBuffer m_ringBuffer;

    // 回读task依赖帧末的FenceValue
    // 但新增回读task时，当前帧的CPU指令还没做完，实际不知道帧末的FenceValue
    // 所以先用pendingTask存一下回读task指针，等帧末FenceValue更新时 同步刷新这里的值
    std::vector<NXReadbackTask*> m_pendingTask;

    // 这里的锁策略是比较简单粗暴的，每个方法都加锁，这些方法的开销都不大。
    // 上传系统的大头开销在BeginTask()结束后，FinishTask()开始前这段时间的各种操作上，而这些操作是暴露在上层，允许多线程同时调用的。
    std::mutex m_mutex;
    std::condition_variable m_condition;
};
