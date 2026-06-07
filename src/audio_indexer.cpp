#include <cstddef>
#include <string>
#include <vector>
#include <unordered_map>
#include "preprocessor.hpp"
#include "spectral_analyzer.hpp"
#include "fingerprint_generator.hpp"
#include "db_handler.hpp"
#include "audio_indexer.hpp"

AudioIndexer::AudioIndexer( const uint32_t downsmp_freq ,
                            const std::string& db_path  ,
                            const std::size_t fir_coefs ,
                            const float frame_duration  ,
                            const uint32_t feature_ratio,
                            const uint8_t window_size   ,
                            const uint8_t peak_number   ):
    downsmp_freq_(downsmp_freq)                                                     ,
    fir_coefs_(fir_coefs)                                                           ,
    spectral_analyzer_(SpectralAnalyzer(frame_duration, downsmp_freq, feature_ratio)),
    fingerprint_generator_(FingerprintGenerator(window_size, peak_number))           ,
    db_handler_(DBHandler(db_path))
{}

void AudioIndexer::index(const std::string& file_path, const std::string& out_path)
{
    const uint32_t sample_rate = Preprocessor::getFileSampleRate(file_path);
    preprocessor_map_.try_emplace(sample_rate, sample_rate, downsmp_freq_, fir_coefs_);

    std::vector<float> prep_signal = preprocessor_map_.at(sample_rate).preprocessData(file_path, out_path);
    std::vector<std::vector<std::size_t>> features = spectral_analyzer_.analyze(prep_signal);
    std::unordered_map<std::size_t, std::vector<uint32_t>> hashes = fingerprint_generator_.genFP(features);
    db_handler_.insertFingerprints(file_path, hashes);
}
