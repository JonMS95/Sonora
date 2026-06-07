#ifndef AUDIO_INDEXER_HPP
#define AUDIO_INDEXER_HPP

#include <cstddef>
#include <unordered_map>
#include "preprocessor.hpp"
#include "spectral_analyzer.hpp"
#include "fingerprint_generator.hpp"
#include "db_handler.hpp"

class AudioIndexer
{
private:
    const uint32_t downsmp_freq_;
    const std::size_t fir_coefs_;

    std::unordered_map<uint32_t, Preprocessor> preprocessor_map_;   // Unique per original sampling frequency.
    SpectralAnalyzer spectral_analyzer_;                            // Unique
    FingerprintGenerator fingerprint_generator_;                    // Unique
    DBHandler db_handler_;                                          // Unique

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
