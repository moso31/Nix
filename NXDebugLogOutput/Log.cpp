#define NXLOG_EXPORTS
#include "Log.h"

void NXPrint::Init()
{

}

void NXPrint::Write(const char* format, ...)
{
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
