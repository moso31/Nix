#pragma once
#include <filesystem>
#include <chrono>

class NXFilesystemHelper
{
public:
    static std::chrono::year_month_day GetLatestFileModifiedTime(const std::filesystem::path& dir_path);
};
