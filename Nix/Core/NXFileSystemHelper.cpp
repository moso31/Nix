#include "NXFilesystemHelper.h"
#include <filesystem>

std::chrono::system_clock::time_point NXFilesystemHelper::get_latest_file_modification_time(const std::filesystem::path& dir_path)
{
    //namespace fs = std::filesystem;

    //if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) 
    //    throw std::runtime_error("Error: Not a valid directory.");

    //std::chrono::system_clock::time_point latest_modification_time;
    //for (const auto& entry : fs::directory_iterator(dir_path)) 
    //{
    //    if (fs::is_regular_file(entry.path())) 
    //    {
    //        auto current_modification_time = fs::last_write_time(entry);
    //        auto current_modification_time_system_clock = std::chrono::time_point_cast<std::chrono::system_clock::duration>(current_modification_time);

    //        if (current_modification_time_system_clock > latest_modification_time) {
    //            latest_modification_time = current_modification_time_system_clock;
    //        }
    //    }
    //}

    return latest_modification_time;
}
