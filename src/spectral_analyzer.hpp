#ifndef SPECTRAL_ANALYZER_HPP
#define SPECTRAL_ANALYZER_HPP

#include <vector>
#include <fftw3.h>
#include "fft_processor.hpp"

class SpectralAnalyzer
{
private:
    const int samples_in_frame_;    // Samples per frame.
    FFTProcessor fft_proc_;         // FFT processor.

    std::vector<std::vector<float>> _split(const std::vector<float>& signal) const;
    void                            _hannSingleFrame(std::vector<float>& frame) const;
    void                            _hann(std::vector<std::vector<float>>& split_signal) const;
    std::vector<float>              _featsSingleFrame(std::vector<float>& frame);
    std::vector<std::vector<float>> _feats(std::vector<std::vector<float>>& split_signal);

public:
    SpectralAnalyzer(const float frame_duration, const int sampling_frequency, const int feature_ratio);

    void analyze(std::vector<float>& signal);
};

#endif