#ifndef SPECTRAL_ANALYZER_HPP
#define SPECTRAL_ANALYZER_HPP

#include <vector>
#include <cstddef>
#include <fftw3.h>
#include "fft_processor.hpp"

class SpectralAnalyzer
{
private:
    const std::size_t samples_in_frame_;    // Samples per frame.
    FFTProcessor fft_proc_;                 // FFT processor.

    std::vector<std::vector<float>>         _split(const std::vector<float>& signal) const              ;
    void                                    _hannSingleFrame(std::vector<float>& frame) const           ;
    void                                    _hann(std::vector<std::vector<float>>& split_signal) const  ;
    std::vector<std::size_t>                _featsSingleFrame(std::vector<float>& frame)                ;
    std::vector<std::vector<std::size_t>>   _feats(std::vector<std::vector<float>>& split_signal)       ;

public:
    SpectralAnalyzer(   const float frame_duration          ,
                        const uint32_t sampling_frequency   ,
                        const uint32_t feature_ratio        );

    std::vector<std::vector<std::size_t>> analyze(std::vector<float>& signal);
};

#endif