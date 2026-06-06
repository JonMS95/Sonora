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
    std::unordered_map<uint32_t, Preprocessor> preprocessor_map;    // Unique per original sampling frequency.
    SpectralAnalyzer spectral_analyzer;                             // Unique
    FingerprintGenerator fingerprint_generator;                     // Unique
    DBHandler db_handler;                                           // Unique

public:
    AudioIndexer(   const uint32_t downsmp_freq ,
                    const std::size_t fir_coefs ,
                    const float frame_duration  ,
                    const uint32_t feature_ratio,
                    const uint8_t window_size   ,
                    const uint8_t peak_number   ,
                    const std::string& db_path  );
};

#endif
