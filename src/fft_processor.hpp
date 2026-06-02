#ifndef FFT_PROCESSOR_HPP
#define FFT_PROCESSOR_HPP

#include <vector>
#include <fftw3.h>

class FFTProcessor
{
private:
    fftwf_complex* fft_comp_;           // Complex FFT pointer.
    fftwf_plan fft_plan_;               // FFT plan.
    float* frame_ptr_;                  // Target frame to be reused by FFT process.
    std::vector<float> sq_mags_;        // Squared magnitudes vector to be used by single-frame FFT.
    const int samples_in_frame_;        // Samples per frame.
    const int feat_ratio_;              // Feature ratio (number of surrounding samples to be discarded when sleecting a peak squared magnitude).
    const int num_of_bins_;             // Number of elements in FFT.
    const std::vector<int> indices_;    // Indices vector to be used as template when extracting features.

    bool _isSqMagIndexValid(const int idx) const;
    bool _isLocalMax(const int idx) const;

public:
    FFTProcessor(const float frame_duration, const int sampling_frequency, const int feature_ratio);
    virtual ~FFTProcessor(void);

    std::vector<float> FFT(const std::vector<float>& frame);
    std::vector<float> featExt(const std::vector<float>& frame);
};

#endif
