#pragma once

// 2023.5.26
// ע�����������ݵ�ע�ͼ���-��-ȫ-��-chatgpt ����ġ��ش˸�л��:)
// This class from the implement to the comments was almost entirely created by ChatGPT. Special thanks! :)
template<class T>
class NXInstance
{
// ���캯����������������Ϊprotected���Է�ֹ�����ഴ����ɾ�������ʵ��
protected:
    NXInstance() {}
    virtual ~NXInstance() {} // ����virtual��Ϊ�˷�ֹ������������ʱ��������

    // ɾ���˿������캯���Ϳ�����ֵ���������ֹ���ʵ�������ƻ�ֵ
    NXInstance(const NXInstance&) = delete;
    NXInstance& operator=(const NXInstance&) = delete;

public:
    static T* GetInstance()
    {
        // ʹ��C++11�ľ�̬�ֲ��������ԣ���֤�ڶ��̻߳����µ��̰߳�ȫ
        static T pInstance;
        return &pInstance;
    }
};
