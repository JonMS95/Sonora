#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>
#include "audio_base.hpp"
#include "audio_indexer.hpp"

AudioIndexer::AudioIndexer( const uint32_t downsmp_freq ,
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
    audio_db_indexer_(AudioDBIndexer(   db_path         ,
                                        downsmp_freq    ,
                                        fir_coefs       ,
                                        frame_duration  ,
                                        feature_ratio   ,
                                        window_size     ,
                                        peak_number     ))
{}

void AudioIndexer::index(const std::string& file_path, const std::string& out_path)
{
    const std::unordered_map<std::size_t, std::vector<uint32_t>> hashes = _getHashes(file_path, out_path);
    audio_db_indexer_.insertFingerprints(file_path, hashes);
}
