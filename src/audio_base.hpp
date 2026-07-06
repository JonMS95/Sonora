#ifndef AUDIO_BASE_HPP
#define AUDIO_BASE_HPP

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include "preprocessor.hpp"
#include "spectral_analyzer.hpp"
#include "fingerprint_generator.hpp"

class AudioBase
{
private:
    const uint32_t downsmp_freq_;
    const std::size_t fir_coefs_;

    std::unordered_map<uint32_t, Preprocessor> preprocessor_map_;   // Unique per original sampling frequency.
    SpectralAnalyzer spectral_analyzer_;                            // Unique
    FingerprintGenerator fingerprint_generator_;                    // Unique

protected:
    uint32_t _getAndCreatePreprocessor(const uint32_t sample_rate);
    uint32_t _getAndCreatePreprocessor(const std::string& file_path);
    std::unordered_map<std::size_t, std::vector<uint32_t>> _getHashes(const std::string& file_path, const std::string& out_path = "");

public:
    explicit AudioBase( const uint32_t downsmp_freq     = 16000 ,
                        const std::size_t fir_coefs     = 101   ,
                        const float frame_duration      = 0.02f ,
                        const uint32_t feature_ratio    = 20    ,
                        const uint8_t window_size       = 5     ,
                        const uint8_t peak_number       = 3     );
};

#endif