#ifndef AUDIO_DB_MATCHER
#define AUDIO_DB_MATCHER

#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include "audio_db_base.hpp"

class AudioDBMatcher : public AudioDBBase
{
private:
    std::string _getSongName(const uint32_t song_id) const;

    void _manageParametersTable(const uint32_t downsmp_freq ,
                                const std::size_t fir_coefs ,
                                const float frame_duration  ,
                                const uint32_t feature_ratio,
                                const uint8_t window_size   ,
                                const uint8_t peak_number   ) const override;

public:
    explicit AudioDBMatcher(const std::string& db_path  ,
                            const uint32_t downsmp_freq ,
                            const std::size_t fir_coefs ,
                            const float frame_duration  ,
                            const uint32_t feature_ratio,
                            const uint8_t window_size   ,
                            const uint8_t peak_number   );

    std::string queryHashes(const std::unordered_map<std::size_t, std::vector<uint32_t>>& frame_hashes) const;
};

#endif