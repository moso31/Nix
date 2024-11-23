#define NXLOG_EXPORTS
#include "Log.h"

void NXPrint::Init()
{

}

void NXPrint::Write(const char* format, ...)
{
    static std::mutex logMutex; // 确保线程安全
    std::lock_guard<std::mutex> lock(logMutex);

    // 获取线程ID字符串表示
    std::stringstream ss;
    ss << std::this_thread::get_id();
    std::string threadIdStr = ss.str();

    // 处理可变参数
    va_list args;
    va_start(args, format);

    // 格式化输出
    printf("[%s]: ", threadIdStr.c_str());
    vprintf(format, args);

    va_end(args);
}
