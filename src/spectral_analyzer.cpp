#include <vector>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <fftw3.h>
#include "spectral_analyzer.hpp"

SpectralAnalyzer::SpectralAnalyzer(const float frame_duration, const int sampling_frequency, const int feature_ratio):
    samples_in_frame_(frame_duration * sampling_frequency),
    fft_proc_(FFTProcessor(frame_duration, sampling_frequency, feature_ratio))
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
    if(static_cast<int>(frame.size()) != samples_in_frame_)
        throw std::runtime_error("Frame size mismatch in FFT");

    if (samples_in_frame_ <= 1)
        return;

    for (int smp_idx = 0; smp_idx < samples_in_frame_; smp_idx++)
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

std::vector<int> SpectralAnalyzer::_featsSingleFrame(std::vector<float>& frame)
{
    return fft_proc_.featExt(frame);
}

std::vector<std::vector<int>> SpectralAnalyzer::_feats(std::vector<std::vector<float>>& split_signal)
{
    const int split_signal_size = static_cast<int>(split_signal.size());
    std::vector<std::vector<int>> ret(split_signal_size);

    for(int f_idx = 0; f_idx < split_signal_size; f_idx++)
        ret[f_idx] = _featsSingleFrame(split_signal[f_idx]);

    return ret;
}

#include <iostream>

std::vector<std::vector<int>> SpectralAnalyzer::analyze(std::vector<float>& signal)
{
    std::vector<std::vector<float>> split_signal = _split(signal);
    _hann(split_signal);
    std::vector<std::vector<int>> ret = _feats(split_signal);
    
    int counter = 0;

    for(const std::vector<int>& vi : ret)
    {
        std::cout << counter++ << ": ";

        for(const int& i : vi)
            std::cout << i << "\t" << " ";
    
        std::cout << std::endl;
    }
}
