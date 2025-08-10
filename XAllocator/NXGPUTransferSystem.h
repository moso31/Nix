#pragma once
#include "XAllocCommon.h"

namespace ccmem
{
    enum class NXTransferType
    {
        None,
        Upload,
        Readback
    };

    class NXRingBuffer;
    struct NXTransferTask
    {
        NXTransferTask();

        void Reset()
        {
            type = NXTransferType::None;
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

        NXTransferType type = NXTransferType::None;
        NXRingBuffer* pRingBuffer = nullptr;
	};

	struct NXTransferContext
    {
        NXTransferContext(const std::string& name) : name(name) {}

        NXTransferTask* pOwner = nullptr;

        // ����ringBuffer����ʱ��Դ���塢��ʱ��Դ�ϴ���ӳ�䡢��ʱ��Դ�ϴ���ƫ����
		ID3D12Resource* pResource = nullptr;
		uint8_t* pResourceData = nullptr;
        uint32_t pResourceOffset = 0;
        std::string name;
	};

    class NXRingBuffer
    {
    public:
        NXRingBuffer(ID3D12Device* pDevice, uint32_t bufferSize, NXTransferType type);
        ~NXRingBuffer();

        bool CanAlloc(uint32_t byteSize);
        bool Build(uint32_t byteSize, NXTransferTask& oTask);
        void Finish(const NXTransferTask& task);
        
        ID3D12Resource* GetResource() { return m_pResource; }
        uint8_t* GetResourceMappedData() { return m_pResourceData; }

    private:
        NXTransferType m_type;
        uint32_t m_size;

        // ��¼ring�е��ѷ��䷶Χ
        uint32_t m_usedStart;
        uint32_t m_usedEnd;

        ID3D12Device* m_pDevice;

        ID3D12Resource* m_pResource;
        uint8_t* m_pResourceData;
    };

    class NXGPUTransferSystem
    {
        const static uint32_t TASK_NUM = 16;

    public:
        NXGPUTransferSystem(ID3D12Device* pDevice);
        ~NXGPUTransferSystem();

        bool BuildTask(int byteSize, NXTransferType taskType, NXTransferContext& taskResult);
        void FinishTask(const NXTransferContext& result, const std::function<void()>& pCallBack = nullptr);
        void Update();
		void SetSyncCommandQueue(ID3D12CommandQueue* pQueue) { m_pSyncCmdQueue = pQueue; }

    private:
        ID3D12Device* m_pDevice;
        ID3D12CommandQueue* m_pCmdQueue;
        ID3D12Fence* m_pFence;
        uint64_t m_fenceValue = 0;

        ID3D12CommandQueue* m_pSyncCmdQueue;

        NXTransferTask m_transferTask[TASK_NUM];

        uint32_t m_taskStart = 0;
        uint32_t m_taskUsed = 0;

        NXRingBuffer m_ringBufferUpload;
        NXRingBuffer m_ringBufferReadback;

        // ������������ǱȽϼ򵥴ֱ��ģ�ÿ����������������Щ�����Ŀ���������
        // �ϴ�ϵͳ�Ĵ�ͷ������BeginTask()������FinishTask()��ʼǰ���ʱ��ĸ��ֲ����ϣ�����Щ�����Ǳ�¶���ϲ㣬������߳�ͬʱ���õġ�
        std::mutex m_mutex;
        std::condition_variable m_condition;
    };
}