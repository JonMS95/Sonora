#include <string>
#include "audio_db_matcher.hpp"
#include "audio_matcher.hpp"

AudioMatcher::AudioMatcher( const uint32_t downsmp_freq ,
                            const std::string& db_path  ,
                            const std::size_t fir_coefs ,
                            const float frame_duration  ,
                            const uint32_t feature_ratio,
                            const uint8_t window_size   ,
                            const uint8_t peak_number   ):
    AudioBase(downsmp_freq, fir_coefs, frame_duration, feature_ratio, window_size, peak_number),
    audio_db_matcher_(AudioDBMatcher(db_path))
{}

std::string AudioMatcher::match(const std::string& file_path) const
{
    return "";
}