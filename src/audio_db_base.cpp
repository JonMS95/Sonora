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

/// @brief Checks whether the parameters table (with a single record) exists.
bool AudioDBBase::_parametersTableExists(void) const
{
    const std::string& select_sql = "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'parameters';";

    sqlite3_stmt* select_stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_, select_sql.c_str(), -1, &select_stmt, nullptr);
    if (rc != SQLITE_OK)
        throw std::runtime_error("Failed SELECT");

    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> p_select_stmt(select_stmt, &sqlite3_finalize);

    rc = sqlite3_step(p_select_stmt.get());
    
    return (rc == SQLITE_ROW);
}

void AudioDBBase::_checkParametersTable(const uint32_t downsmp_freq ,
                                        const std::size_t fir_coefs ,
                                        const float frame_duration  ,
                                        const uint32_t feature_ratio,
                                        const uint8_t window_size   ,
                                        const uint8_t peak_number   ) const
{
    // Name columns explicitly instead of using '*' so as to avoid potential mismatches in the future.
    const std::string& select_sql = "SELECT downsmp_freq, fir_coefs, frame_duration, feature_ratio, window_size, peak_number FROM parameters;";

    sqlite3_stmt* select_stmt = nullptr;

    int rc = sqlite3_prepare_v2(db_, select_sql.c_str(), -1, &select_stmt, nullptr);
    if (rc != SQLITE_OK)
        throw std::runtime_error("Failed SELECT");

    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> p_select_stmt(select_stmt, &sqlite3_finalize);

    rc = sqlite3_step(p_select_stmt.get());
    if(rc != SQLITE_ROW)
        throw std::runtime_error("Could not perform SELECT statement over parameters table");

    const uint32_t db_downsmp_freq  = static_cast<uint32_t>(sqlite3_column_int(     p_select_stmt.get(), 0));
    const std::size_t db_fir_coefs  = static_cast<std::size_t>(sqlite3_column_int(  p_select_stmt.get(), 1));
    const float db_frame_duration   = static_cast<float>(sqlite3_column_double(     p_select_stmt.get(), 2));
    const uint32_t db_feature_ratio = static_cast<uint32_t>(sqlite3_column_int(     p_select_stmt.get(), 3));
    const uint8_t db_window_size    = static_cast<uint8_t>(sqlite3_column_int(      p_select_stmt.get(), 4));
    const uint8_t db_peak_number    = static_cast<uint8_t>(sqlite3_column_int(      p_select_stmt.get(), 5));

    if( (db_downsmp_freq    != downsmp_freq)    ||
        (db_fir_coefs       != fir_coefs)       ||
        (db_frame_duration  != frame_duration)  ||
        (db_feature_ratio   != feature_ratio)   ||
        (db_window_size     != window_size)     ||
        (db_peak_number     != peak_number)     )
    {
        auto make_comp_substr = [](const std::string& value_name, auto provided_value, auto db_value) -> std::string
        {
            return std::string(value_name + ": got: " + std::to_string(provided_value) + ", expected: " + std::to_string(db_value) + " (" + ((provided_value == db_value) ? "equal" : "differ") + ")");
        };

        const std::string re_str =  make_comp_substr("Downsampling frequency",              downsmp_freq,   db_downsmp_freq) +  "\r\n" + 
                                    make_comp_substr("Number of FIR filter coefficients",   fir_coefs,      db_fir_coefs) +     "\r\n" + 
                                    make_comp_substr("Downsampling frequency",              downsmp_freq,   db_downsmp_freq) +  "\r\n" + 
                                    make_comp_substr("Feature ratio",                       feature_ratio,  db_feature_ratio) + "\r\n" + 
                                    make_comp_substr("Window size",                         window_size,    db_window_size) +   "\r\n" + 
                                    make_comp_substr("Number of peaks",                     peak_number,    db_peak_number);

        throw std::runtime_error(re_str);
    }
}
