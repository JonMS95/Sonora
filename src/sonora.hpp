#ifndef SONORA_HPP
#define SONORA_HPP

#include <queue>
#include <string>
#include <thread>
#include "audio_indexer.hpp"
#include "audio_matcher.hpp"

// Should we make this a singleton??
class Sonora
{
private:
    AudioIndexer audio_indexer_;
    AudioMatcher audio_matcher_;

    const uint8_t max_index_rqs_;
    const uint8_t max_match_rqs_;
    const uint8_t max_match_ops_;

    std::queue<std::string> index_requests_;
    std::queue<std::string> match_requests_;

    bool index_ongoing_;

    std::jthread index_thread_;
    std::jthread match_thread_;

    void _indexRoutine(void);
    void _matchRoutine(void);

public:
    explicit Sonora(const uint32_t downsmp_freq             ,
                    const std::string& db_path              ,
                    const std::size_t fir_coefs     = 101   ,
                    const float frame_duration      = 0.02f ,
                    const uint32_t feature_ratio    = 20    ,
                    const uint8_t window_size       = 5     ,
                    const uint8_t peak_number       = 3     ,
                    const uint8_t max_index_rqs     = 10    ,
                    const uint8_t max_match_rqs     = 10    ,
                    const uint8_t max_match_ops     = 10    );
    
    bool index(const std::string& file_path);
    std::string match(const std::string& file_path);
};

#endif