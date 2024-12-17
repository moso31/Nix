#pragma once
#include "XAllocCommon.h"

namespace ccmem
{
    struct UploadTaskContext;
    struct UploadTask
    {
        UploadTask();

        void Reset()
        {
			ringPos = 0;
			byteSize = 0;
			fenceValue = 0;
		}

		ID3D12GraphicsCommandList* pCmdList = nullptr;
		ID3D12CommandAllocator* pCmdAllocator = nullptr;

		// 记录 task 的fenceValue
		// 起手 BuildTask()时，fenceValue = -1（表示未完成）
		// 完成时 FinishTask() 方法会刷新这个值为一个正常的值，
		// 然后每帧 Update() 时，GPU才能将其移除
		uint64_t fenceValue = 0;

        // 任务完成后的回调
        std::function<void()> pCallback = nullptr;

		// 在RingBuffer中的位置和大小
		uint32_t ringPos = 0;
		uint32_t byteSize = 0;

        uint64_t selfID;
	};

	struct UploadTaskContext
    {
        UploadTaskContext(const std::string& name) : name(name) {}

        UploadTask* pOwner = nullptr;

        // ringBuffer的临时资源本体、临时资源上传堆映射、临时资源上传堆偏移量
		ID3D12Resource* pResource = nullptr;
		uint8_t* pResourceData = nullptr;
        uint32_t pResourceOffset = 0;
        std::string name;
	};

    class UploadRingBuffer
    {
    public:
        UploadRingBuffer(ID3D12Device* pDevice, uint32_t bufferSize);
        ~UploadRingBuffer();

        bool BuildTask(uint32_t byteSize, UploadTask& oTask);
        void FinishTask(const UploadTask& task);
        
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

    class UploadSystem
    {
        const static uint32_t UPLOADTASK_NUM = 16;

    public:
        UploadSystem(ID3D12Device* pDevice);
        ~UploadSystem();

        bool BuildTask(int byteSize, UploadTaskContext& taskResult);
        void FinishTask(const UploadTaskContext& result, const std::function<void()>& pCallBack = nullptr);
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
        std::condition_variable m_condition;
    };
}