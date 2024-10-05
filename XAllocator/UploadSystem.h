#pragma once
#include "XAllocCommon.h"

namespace ccmem
{
    struct UploadTask
    {
        void Reset()
        {
            isWorking = false;
            ringPos = 0;
            byteSize = 0;
            fenceValue = 0;
        }

        ID3D12GraphicsCommandList* pCmdList;
        ID3D12CommandAllocator* pCmdAllocator;

        // 记录 task 的fenceValue
	    // 起手 BuildTask()时，fenceValue = -1（表示未完成）
	    // 完成时 FinishTask() 方法会刷新这个值为一个正常的值，
        // 然后每帧 Update() 时，GPU才能将其移除
        uint64_t fenceValue;

        // 在RingBuffer中的位置和大小
        uint32_t ringPos;   
        uint32_t byteSize;

        bool isWorking = false;
    };

    struct UploadTaskContext
    {
        UploadTask* inputTask = nullptr;
		ID3D12Resource* pResource = nullptr;
		uint8_t* pResourceData = nullptr;
	};

    class UploadRingBuffer
    {
    public:
        UploadRingBuffer(ID3D12Device* pDevice, uint32_t bufferSize);
        ~UploadRingBuffer();

        bool BuildTask(uint32_t byteSize, UploadTask& oTask);
        
        ID3D12Resource* GetResource() { return m_pResource; }
        uint8_t* GetResourceMappedData() { return m_pResourceData; }

    private:
        uint32_t m_size;

        // 已分配范围指示器。
        uint32_t m_usedStart;
        uint32_t m_usedEnd;

        ID3D12Device* m_pDevice;

        ID3D12Resource* m_pResource;
        uint8_t* m_pResourceData;
    };

    class UploadSystem
    {
        const static uint32_t UPLOADTASK_NUM = 16;

    public:
        UploadSystem(ID3D12Device* pDevice);
        ~UploadSystem();

        bool BuildTask(int byteSize, UploadTaskContext& result);
        void FinishTask(const UploadTaskContext& result);
        void Update();

    private:
        ID3D12Device* m_pDevice;
        ID3D12CommandQueue* m_pCmdQueue;
        ID3D12Fence* m_pFence;
        uint64_t m_fenceValue = 0;

        UploadTask m_uploadTask[UPLOADTASK_NUM];

        uint32_t m_taskStart = 0;
        uint32_t m_taskUsed = 0;

        UploadRingBuffer m_ringBuffer;

        // 这里的锁策略是比较简单粗暴的，每个方法都加锁，这些方法的开销都不大。
        // 上传系统的大头开销在BeginTask()结束后，FinishTask()开始前这段时间的各种操作上，而这些操作是暴露在上层，允许多线程同时调用的。
        std::mutex m_mutex;
    };
}