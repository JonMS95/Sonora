#include <vector>
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <queue>
#include <unordered_set>
#include <numeric>
#include <fftw3.h>
#include "fft_processor.hpp"

#define SAMPLES_IN_FRAME(F_DUR, S_FREQ) (F_DUR * S_FREQ)
#define NUM_OF_BINS(F_DUR, S_FREQ)  (((F_DUR * S_FREQ) / 2) + 1)

static inline fftwf_complex* initFFTComplex(const float frame_duration, const int sampling_frequency)
{
    return static_cast<fftwf_complex*>(fftwf_malloc(sizeof(fftwf_complex) * NUM_OF_BINS(frame_duration, sampling_frequency)));
}

static inline float* initFramePointer(const float frame_duration, const int sampling_frequency)
{
    return static_cast<float*>(fftwf_malloc(sizeof(float) * SAMPLES_IN_FRAME(frame_duration, sampling_frequency)));
}

static std::vector<int> initIndicesTemp(const float frame_duration, const int sampling_frequency)
{
    std::vector<int> ret(NUM_OF_BINS(frame_duration, sampling_frequency));
    std::iota(ret.begin(), ret.end(), 0);

    return ret;
}

FFTProcessor::FFTProcessor(const float frame_duration, const int sampling_frequency, const int feature_ratio):
    fft_comp_(initFFTComplex(frame_duration, sampling_frequency)),
    frame_ptr_(initFramePointer(frame_duration, sampling_frequency)),
    samples_in_frame_(SAMPLES_IN_FRAME(frame_duration, sampling_frequency)),
    feat_ratio_(feature_ratio), 
    sq_mags_(std::vector<float>(NUM_OF_BINS(frame_duration, sampling_frequency))),
    num_of_bins_(NUM_OF_BINS(frame_duration, sampling_frequency)),
    indices_(initIndicesTemp(frame_duration, sampling_frequency))
{
    fft_plan_ = fftwf_plan_dft_r2c_1d(samples_in_frame_, frame_ptr_, fft_comp_, FFTW_ESTIMATE);
}

FFTProcessor::~FFTProcessor(void)
{
    fftwf_destroy_plan(fft_plan_);
    fftwf_free(frame_ptr_);
    fftwf_free(fft_comp_);
}

std::vector<float> FFTProcessor::FFT(const std::vector<float>& frame)
{
    if(frame.size() != samples_in_frame_)
        throw std::runtime_error("Frame size mismatch in FFT");

    std::memcpy(frame_ptr_, frame.data(), sizeof(float) * samples_in_frame_);

    fftwf_execute(fft_plan_);

    for (int k = 0; k < num_of_bins_; k++)
    {
        float re = fft_comp_[k][0];
        float im = fft_comp_[k][1];

        sq_mags_[k] = (re * re + im * im);
    }

    return sq_mags_;
}

bool FFTProcessor::_isSqMagIndexValid(const int idx) const
{
    return ((idx >= 0) && (idx < num_of_bins_));
}

bool FFTProcessor::_isLocalMax(const int idx) const
{
    if(!_isSqMagIndexValid(idx))
        throw std::runtime_error("Provided index is not valid");

    const float cur     = sq_mags_[idx];
    const float prev    = (idx == 0 ? -1.0 : sq_mags_[idx - 1]);
    const float next    = (idx == (num_of_bins_ - 1) ? -1.0 : sq_mags_[idx + 1]);
    
    return ((prev <= cur) && (cur >= next));
}

std::vector<float> FFTProcessor::featExt(const std::vector<float>& frame)
{
    auto sort_crit = [this](const int& a, const int& b) -> bool
    {
        return sq_mags_[a] < sq_mags_[b];
    };

    FFT(frame);

    std::vector<int> idcs = indices_;
    std::priority_queue<int, std::vector<int>, decltype(sort_crit)> max_heap(idcs.begin(), idcs.end(), sort_crit);
    std::unordered_set<int> disc_idx;
    std::vector<float> ret;
    int idx;

    while(!max_heap.empty())
    {
        idx = max_heap.top();

        if(!disc_idx.count(idx) && _isLocalMax(idx))
        {
            ret.emplace_back(sq_mags_[idx]);

            for(int i = (idx - feat_ratio_); i <= (idx + feat_ratio_); i++)
                if(i >= 0 && i < num_of_bins_)
                    disc_idx.insert(i);
        }

        max_heap.pop();
    }

    return ret;
}
