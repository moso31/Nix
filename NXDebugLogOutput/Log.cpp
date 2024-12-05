#define NXLOG_EXPORTS
#include "Log.h"

void NXPrint::Init()
{

}

bool NXPrint::WriteCondition(const int id)
{
    return id == 2;
}

void NXPrint::Write(const int id, const char* format, ...)
{
    if (!WriteCondition(id))
        return;

    static std::mutex logMutex; // ȷ���̰߳�ȫ
    std::lock_guard<std::mutex> lock(logMutex);

    // ��ȡ�߳�ID�ַ�����ʾ
    std::stringstream ss;
    ss << std::this_thread::get_id();
    std::string threadIdStr = ss.str();

    // ����ɱ����
    va_list args;
    va_start(args, format);

    // ��ʽ�����
    printf("[%s]: ", threadIdStr.c_str());
    vprintf(format, args);

    va_end(args);
}
