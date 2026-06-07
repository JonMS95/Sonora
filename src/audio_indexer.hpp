#ifndef AUDIO_INDEXER_HPP
#define AUDIO_INDEXER_HPP

#include <cstddef>
#include <unordered_map>
#include "audio_base.hpp"
#include "audio_db_indexer.hpp"

class AudioIndexer : public AudioBase
{
private:
    AudioDBIndexer audio_db_indexer_; // Unique

public:
    explicit AudioIndexer(  const uint32_t downsmp_freq             ,
                            const std::string& db_path              ,
                            const std::size_t fir_coefs     = 101   ,
                            const float frame_duration      = 0.02f ,
                            const uint32_t feature_ratio    = 20    ,
                            const uint8_t window_size       = 5     ,
                            const uint8_t peak_number       = 3     );

    void index(const std::string& file_path, const std::string& out_path = "");
};

#endif
