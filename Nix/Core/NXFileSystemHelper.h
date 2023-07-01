#pragma once

#include <filesystem>
#include <chrono>

class NXFilesystemHelper
{
public:
    static std::chrono::system_clock::time_point get_latest_file_modification_time(const std::filesystem::path& dir_path);
};
