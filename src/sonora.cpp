#include <cstddef>
#include <queue>
#include <string>
#include "audio_indexer.hpp"
#include "audio_matcher.hpp"
#include "sonora.hpp"

Sonora::Sonora( const uint32_t downsmp_freq ,
                const std::string& db_path  ,
                const std::size_t fir_coefs ,
                const float frame_duration  ,
                const uint32_t feature_ratio,
                const uint8_t window_size   ,
                const uint8_t peak_number   ,
                const uint8_t max_index_rqs ,
                const uint8_t max_match_rqs ,
                const uint8_t max_match_ops ):
    audio_indexer_(AudioIndexer(downsmp_freq    ,
                                db_path         ,
                                fir_coefs       ,
                                frame_duration  ,
                                feature_ratio   ,
                                window_size     ,
                                peak_number     )),
    audio_matcher_(AudioMatcher(downsmp_freq    ,
                                db_path         ,
                                fir_coefs       ,
                                frame_duration  ,
                                feature_ratio   ,
                                window_size     ,
                                peak_number     )),
    max_index_rqs_(max_index_rqs),
    max_match_rqs_(max_match_rqs),
    max_match_ops_(max_match_ops),
    index_ongoing_(false)
{}

bool Sonora::index(const std::string& file_path)
{
    if(static_cast<uint8_t>(index_requests_.size()) >= max_index_rqs_)
        return false;
    
    index_requests_.push(file_path);

    return true;
}

bool Sonora::match(const std::string& file_path)
{
    if(static_cast<uint8_t>(match_requests_.size()) >= max_match_rqs_)
        return false;
    
    match_requests_.push(file_path);

    return true;
}
