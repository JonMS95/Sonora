#include <cmath>
#include <stdexcept>
#include "fir_filter.hpp"

/// @brief Creates a basic FIR filter.
/// @param size Number of weights.
/// @param cutoff Normalized cutoff frequency (Fc / Fs). Must be in the range (0.0, 0.5).
/// @return A vectorwith FIR filter coefficients.
FIRFilter::FIRFilter(const float cutoff, const std::size_t filter_size): kernel_(std::vector<float>(filter_size))
{
    if(filter_size == 0)
        throw std::invalid_argument("FIR filter cannot have zero coefficients");

    if(cutoff <= 0.0f || cutoff >= 0.5f)
        throw std::invalid_argument("Cutoff coefficient must be in the range (0.0, 0.5) non-inclusive, received: " + std::to_string(cutoff));

    std::size_t M = filter_size / 2;
    float sum = 0.0f;

    for (std::size_t n = 0; n < filter_size; n++)
    {
        std::size_t x = n - M;

        // normalized cutoff (0..0.5)
        float fc = cutoff;

        float sinc = (x == 0)
            ? 2.0f * fc
            : sinf(2.0f * M_PI * fc * x) / (M_PI * x);

        // Hann window
        float window = 0.5f - 0.5f * cosf(2.0f * M_PI * n / (filter_size - 1));

        kernel_[n] = sinc * window;
        sum += kernel_[n];
    }

    // normalize
    for (float& v : kernel_)
        v /= sum;
}

/// @brief Applies fir filter over a signal.
/// @param signal Signal to be filtered.
/// @param kernel FIR filter kernel.
/// @return Processed (low-pass-filtered) signal.
std::vector<float> FIRFilter::applyFIR(const std::vector<float>& signal) const
{
    std::vector<float> out(signal.size());

    std::size_t N = signal.size();
    std::size_t K = kernel_.size();
    std::size_t M = K / 2;

    float acc; // Cumulative counter.
    std::size_t idx;

    for (std::size_t i = 0; i < N; i++)
    {
        acc = 0.0f;

        for (std::size_t j = 0; j < K; j++)
        {
            idx = i + j - M;

            if (idx < N)
                acc += signal[idx] * kernel_[j];
        }

        out[i] = acc;
    }

    return out;
}
