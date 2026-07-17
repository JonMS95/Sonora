#include <cmath>
#include <stdexcept>
#include "spectral_analyzer.hpp"

SpectralAnalyzer::SpectralAnalyzer( const float frame_duration          ,
                                    const uint32_t sampling_frequency   ,
                                    const uint32_t feature_ratio        ,
                                    const uint8_t peak_number           ):
    samples_in_frame_(frame_duration * sampling_frequency)  ,
    fft_proc_(FFTProcessor( frame_duration                  ,
                            sampling_frequency              ,
                            feature_ratio                   ,
                            peak_number                     ))
{}

std::vector<std::vector<float>> SpectralAnalyzer::_split(const std::vector<float>& signal) const
{
    std::vector<std::vector<float>> ret;

    for (std::size_t i = 0; i + samples_in_frame_ <= signal.size(); i += samples_in_frame_)
        ret.emplace_back(signal.begin() + i, signal.begin() + i + samples_in_frame_);

    return ret;
}

void SpectralAnalyzer::_hannSingleFrame(std::vector<float>& frame) const
{
    if(frame.size() != samples_in_frame_)
        throw std::runtime_error("Frame size mismatch in FFT");

    if (samples_in_frame_ <= 1)
        return;

    for (std::size_t smp_idx = 0; smp_idx < samples_in_frame_; smp_idx++)
    {
        float w = 0.5f - 0.5f * std::cos((2.0f * M_PI * smp_idx) / (samples_in_frame_ - 1));
        frame[smp_idx] *= w;
    }
}

void SpectralAnalyzer::_hann(std::vector<std::vector<float>>& split_signal) const 
{
    for(std::vector<float>& frame : split_signal)
        _hannSingleFrame(frame);
}

std::vector<std::size_t> SpectralAnalyzer::_featsSingleFrame(std::vector<float>& frame)
{
    return fft_proc_.featExt(frame);
}

std::vector<std::vector<std::size_t>> SpectralAnalyzer::_feats(std::vector<std::vector<float>>& split_signal)
{
    const std::size_t split_signal_size = split_signal.size();
    std::vector<std::vector<std::size_t>> ret(split_signal_size);

    for(std::size_t f_idx = 0; f_idx < split_signal_size; f_idx++)
        ret[f_idx] = _featsSingleFrame(split_signal[f_idx]);

    return ret;
}

std::vector<std::vector<std::size_t>> SpectralAnalyzer::analyze(std::vector<float>& signal)
{
    std::vector<std::vector<float>> split_signal = _split(signal);
    _hann(split_signal);
    return _feats(split_signal);
}
