#ifndef FFT_PROCESSOR_HPP
#define FFT_PROCESSOR_HPP

#include <vector>
#include <cstddef>
#include <fftw3.h>

class FFTProcessor
{
private:
    float* frame_ptr_;                          // Target frame to be reused by FFT process.
    const std::size_t samples_in_frame_;        // Samples per frame.
    const std::size_t feat_ratio_;              // Feature ratio (number of surrounding samples to be discarded when sleecting a peak squared magnitude).
    std::vector<float> sq_mags_;                // Squared magnitudes vector to be used by single-frame FFT.
    const std::size_t num_of_bins_;             // Number of elements in FFT.
    const std::vector<std::size_t> indices_;    // Indices vector to be used as template when extracting features.
    fftwf_complex* fft_comp_;                   // Complex FFT pointer.
    fftwf_plan fft_plan_;                       // FFT plan.

    bool _isSqMagIndexValid(const std::size_t idx) const;
    bool _isLocalMax(const std::size_t idx) const;

public:
    FFTProcessor(   const float frame_duration          ,
                    const uint32_t sampling_frequency   ,
                    const uint32_t feature_ratio        );
    virtual ~FFTProcessor(void);

    std::vector<float> FFT(const std::vector<float>& frame);
    std::vector<std::size_t> featExt(const std::vector<float>& frame);
};

#endif
