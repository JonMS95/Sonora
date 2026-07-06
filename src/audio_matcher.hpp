#ifndef AUDIO_MATCHER_HPP
#define AUDIO_MATCHER_HPP

#include <string>
#include <cstdint>
#include <cstddef>
#include "audio_db_matcher.hpp"
#include "audio_base.hpp"

class AudioMatcher : public AudioBase
{
private:
    AudioDBMatcher audio_db_matcher_; // Unique

public:
    explicit AudioMatcher(  const uint32_t downsmp_freq ,
                            const std::string& db_path  ,
                            const std::size_t fir_coefs ,
                            const float frame_duration  ,
                            const uint32_t feature_ratio,
                            const uint8_t window_size   ,
                            const uint8_t peak_number   );

    std::string match(const std::string& file_path);
};

#endif