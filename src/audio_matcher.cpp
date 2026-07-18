#include "audio_matcher.hpp"

AudioMatcher::AudioMatcher( const uint32_t downsmp_freq ,
                            const std::string& db_path  ,
                            const std::size_t fir_coefs ,
                            const float frame_duration  ,
                            const uint32_t feature_ratio,
                            const uint8_t window_size   ,
                            const uint8_t peak_number   ):
    AudioBase(  downsmp_freq    ,
                fir_coefs       ,
                frame_duration  ,
                feature_ratio   ,
                window_size     ,
                peak_number     ),
    audio_db_matcher_(  db_path         ,
                        downsmp_freq    ,
                        fir_coefs       ,
                        frame_duration  ,
                        feature_ratio   ,
                        window_size     ,
                        peak_number     )
{}

std::string AudioMatcher::match(const std::string& file_path)
{
    const std::unordered_map<std::size_t, std::vector<uint32_t>> hashes = _getHashes(file_path);

    return audio_db_matcher_.queryHashes(hashes);
}