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

        uint64_t selfID;
	};

	struct UploadTaskContext
    {
        UploadTaskContext(const std::string& name) : name(name) {}

        UploadTask* pOwner = nullptr;

        // ringBuffer����ʱ��Դ���塢��ʱ��Դ�ϴ���ӳ�䡢��ʱ��Դ�ϴ���ƫ����
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

        // ��¼ring�е��ѷ��䷶Χ
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

        // ������������ǱȽϼ򵥴ֱ��ģ�ÿ����������������Щ�����Ŀ���������
        // �ϴ�ϵͳ�Ĵ�ͷ������BeginTask()������FinishTask()��ʼǰ���ʱ��ĸ��ֲ����ϣ�����Щ�����Ǳ�¶���ϲ㣬������߳�ͬʱ���õġ�
        std::mutex m_mutex;
        std::condition_variable m_condition;
    };
}