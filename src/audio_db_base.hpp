#ifndef AUDIO_DB_BASE_HPP
#define AUDIO_DB_BASE_HPP

#include <string>
#include <cstdint>
#include <cstddef>
#include <sqlite3.h>

class AudioDBBase
{
protected:
    sqlite3* db_;

    bool _parametersTableExists(void) const;
    void _checkParametersTable( const uint32_t downsmp_freq ,
                                const std::size_t fir_coefs ,
                                const float frame_duration  ,
                                const uint32_t feature_ratio,
                                const uint8_t window_size   ,
                                const uint8_t peak_number   ) const;
    void _enableWAL(void) const;

    // Any derived class hould take parameters table into account.
    virtual void _manageParametersTable(const uint32_t downsmp_freq ,
                                        const std::size_t fir_coefs ,
                                        const float frame_duration  ,
                                        const uint32_t feature_ratio,
                                        const uint8_t window_size   ,
                                        const uint8_t peak_number   ) const = 0;

public:
    explicit AudioDBBase(const std::string& db_path, const bool exact_path);
    virtual ~AudioDBBase(void);
};


#endif
