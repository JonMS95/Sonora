#include <string>
#include <filesystem>
#include <sqlite3.h>
#include "audio_db_base.hpp"

AudioDBBase::AudioDBBase(const std::string& db_path): db_(nullptr)
{
    // Check whether the path to the target database exists.
    std::filesystem::path fs_db_path(db_path);

    if(fs_db_path.has_parent_path())
    {
        if(!std::filesystem::exists(fs_db_path.parent_path()))
            throw std::runtime_error("Database directory does not exist: " + fs_db_path.parent_path().string());
        
        if(!std::filesystem::is_directory(fs_db_path.parent_path()))
            throw std::runtime_error("Path to database is not a directory");
    }

    int ret = sqlite3_open(db_path.c_str(), &db_);

    if(ret != SQLITE_OK)
    {
        const std::string err_str = sqlite3_errmsg(db_);
        sqlite3_close(db_);

        throw std::runtime_error("SQLite open failed: "+ err_str);
    }
}

AudioDBBase::~AudioDBBase(void)
{
    sqlite3_close_v2(db_);
}
