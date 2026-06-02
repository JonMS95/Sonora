#include <vector>
#include <cmath>
#include "fir_filter.hpp"

/// @brief Creates a basic FIR filter.
/// @param size Number of weights.
/// @param cutoff Cutoff coefficient value, calculated as: 0.5 * (desired cutoff frequency) / (original maximum or sampling frequency).
/// @return A vectorwith FIR filter coefficients.
FIRFilter::FIRFilter(const float cutoff, const int size): kernel_(std::vector<float>(size))
{
    int M = size / 2;
    float sum = 0.0f;

    for (int n = 0; n < size; n++)
    {
        int x = n - M;

        // normalized cutoff (0..0.5)
        float fc = cutoff;

        float sinc = (x == 0)
            ? 2.0f * fc
            : sinf(2.0f * M_PI * fc * x) / (M_PI * x);

        // Hann window
        float window = 0.5f - 0.5f * cosf(2.0f * M_PI * n / (size - 1));

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

    int N = signal.size();
    int K = kernel_.size();
    int M = K / 2;

    for (int i = 0; i < N; i++)
    {
        float acc = 0.0f;

        for (int j = 0; j < K; j++)
        {
            int idx = i + j - M;

            if (idx >= 0 && idx < N)
                acc += signal[idx] * kernel_[j];
        }

        out[i] = acc;
    }

    return out;
}
