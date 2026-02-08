#include "NXFilesystemHelper.h"

std::chrono::year_month_day NXFilesystemHelper::GetLatestFileModifiedTime(const std::filesystem::path& directoryPath)
{
    namespace fs = std::filesystem;
    using namespace std::chrono;

    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath))
        throw std::runtime_error("Error: Not a valid directory.");

    // 获取最后写入时间，文件夹下所有文件取最新
    std::filesystem::file_time_type lastModifyTime;
    for (const auto& entry : fs::directory_iterator(directoryPath))
    {
        if (fs::is_regular_file(entry.path())) 
        {
            auto lastWriteTime = fs::last_write_time(entry.path());
            if (lastWriteTime > lastModifyTime)
				lastModifyTime = lastWriteTime;
        }
    }

    // 转换为std::chrono::system_clock::time_point
    auto sys_time = time_point_cast<system_clock::duration>(lastModifyTime - std::filesystem::file_time_type::clock::now() + system_clock::now());

    // 转换为std::chrono::year_month_day
    return year_month_day(floor<days>(sys_time));
}
