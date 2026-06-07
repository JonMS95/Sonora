#include "audio_base.hpp"

AudioBase::AudioBase(   const uint32_t downsmp_freq ,
                        const std::size_t fir_coefs ,
                        const float frame_duration  ,
                        const uint32_t feature_ratio,
                        const uint8_t window_size   ,
                        const uint8_t peak_number   ):
    downsmp_freq_(downsmp_freq)                                                         ,
    fir_coefs_(fir_coefs)                                                               ,
    spectral_analyzer_(SpectralAnalyzer(frame_duration, downsmp_freq, feature_ratio))   ,
    fingerprint_generator_(FingerprintGenerator(window_size, peak_number))
{}

uint32_t AudioBase::_getAndCreatePreprocessor(const uint32_t sample_rate)
{
    preprocessor_map_.try_emplace(sample_rate, sample_rate, downsmp_freq_, fir_coefs_);
    return sample_rate;
}

uint32_t AudioBase::_getAndCreatePreprocessor(const std::string& file_path)
{
    return _getAndCreatePreprocessor(Preprocessor::getFileSampleRate(file_path));
}

std::unordered_map<std::size_t, std::vector<uint32_t>> AudioBase::_getHashes(const std::string& file_path, const std::string& out_path)
{
    const uint32_t sample_rate = _getAndCreatePreprocessor(file_path);

    std::vector<float> prep_signal = preprocessor_map_.at(sample_rate).preprocessData(file_path, out_path);
    std::vector<std::vector<std::size_t>> features = spectral_analyzer_.analyze(prep_signal);
    return fingerprint_generator_.genFP(features);
}

