#include <stdexcept>
#include <cstring>
#include <cstddef>
#include <cmath>
#include <queue>
#include <numeric>
#include <unordered_set>
#include "fft_processor.hpp"

constexpr std::size_t getSamplesPerFrame(const float frame_duration, const uint32_t sampling_frequency)
{
    return static_cast<std::size_t>(frame_duration * sampling_frequency);
}

constexpr std::size_t getNumberOfBins(const float frame_duration, const uint32_t sampling_frequency)
{
    return ((getSamplesPerFrame(frame_duration, sampling_frequency) / 2) + 1);
}

static inline fftwf_complex* initFFTComplex(const std::size_t number_of_bins)
{
    return static_cast<fftwf_complex*>(fftwf_malloc(sizeof(fftwf_complex) * number_of_bins));
}

static inline float* initFramePointer(const std::size_t samples_per_frame)
{
    return static_cast<float*>(fftwf_malloc(sizeof(float) * samples_per_frame));
}

static std::vector<std::size_t> initIndicesTemp(const std::size_t number_of_bins)
{
    std::vector<std::size_t> ret(number_of_bins);
    std::iota(ret.begin(), ret.end(), 0);

    return ret;
}

FFTProcessor::Config FFTProcessor::makeConfig(  const float frame_duration          ,
                                                const uint32_t sampling_frequency   ,
                                                const uint32_t feature_ratio        )
{
    if(frame_duration <= 0)
        throw std::invalid_argument("Frame duration can never be lower or equal than zero");
    
    if(sampling_frequency == 0)
        throw std::invalid_argument("Sampling frequency can never be equal to zero");

    if(feature_ratio == 0)
        throw std::invalid_argument("Feature ratio can never be equal to zero");

    Config ret =
    {
        .samples_per_frame  = getSamplesPerFrame(frame_duration, sampling_frequency),
        .number_of_bins     = getNumberOfBins(frame_duration, sampling_frequency)   ,
        .feature_ratio      = feature_ratio                                         ,
    };

    return ret;
}

FFTProcessor::FFTProcessor( const float frame_duration          ,
                            const uint32_t sampling_frequency   ,
                            const uint32_t feature_ratio        ):
    FFTProcessor(FFTProcessor::makeConfig(  frame_duration      ,
                                            sampling_frequency  ,
                                            feature_ratio)      )
{}

FFTProcessor::FFTProcessor(const Config& cfg):
    frame_ptr_(initFramePointer(cfg.samples_per_frame), &fftwf_free),
    samples_in_frame_(cfg.samples_per_frame)                        ,
    feat_ratio_(cfg.feature_ratio)                                  , 
    sq_mags_(std::vector<float>(cfg.number_of_bins))                ,
    num_of_bins_(cfg.number_of_bins)                                ,
    indices_(initIndicesTemp(cfg.number_of_bins))                   ,
    fft_comp_(initFFTComplex(cfg.number_of_bins), &fftwf_free)
{
    fft_plan_ = fftwf_plan_dft_r2c_1d(cfg.samples_per_frame, frame_ptr_.get(), fft_comp_.get(), FFTW_ESTIMATE);
}

FFTProcessor::~FFTProcessor(void)
{
    fftwf_destroy_plan(fft_plan_);
}

std::vector<float> FFTProcessor::computePowerSpectrum(const std::vector<float>& frame)
{
    if(frame.size() != samples_in_frame_)
        throw std::invalid_argument("Frame size mismatch in FFT, got: " + std::to_string(frame.size()) + ", expected: " + std::to_string(samples_in_frame_));

    std::memcpy(frame_ptr_.get(), frame.data(), sizeof(float) * samples_in_frame_);

    fftwf_execute(fft_plan_);

    for (std::size_t k = 0; k < num_of_bins_; k++)
    {
        float re = fft_comp_.get()[k][0];
        float im = fft_comp_.get()[k][1];

        sq_mags_[k] = (re * re + im * im);
    }

    return sq_mags_;
}

bool FFTProcessor::_isSqMagIndexValid(const std::size_t idx) const
{
    return (idx < num_of_bins_);
}

bool FFTProcessor::_isLocalMax(const std::size_t idx) const
{
    if(!_isSqMagIndexValid(idx))
        throw std::runtime_error("Provided index is not valid");

    const float cur     = sq_mags_[idx];
    const float prev    = (idx == 0 ? -1.0 : sq_mags_[idx - 1]);
    const float next    = (idx == (num_of_bins_ - 1) ? -1.0 : sq_mags_[idx + 1]);
    
    return ((prev <= cur) && (cur >= next));
}

std::vector<std::size_t> FFTProcessor::featExt(const std::vector<float>& frame)
{
    auto sort_crit = [this](const std::size_t& a, const std::size_t& b) -> bool
    {
        return sq_mags_[a] < sq_mags_[b];
    };

    std::lock_guard<std::mutex> lock(fft_mtx_);

    computePowerSpectrum(frame);

    std::vector<std::size_t> idcs = indices_;
    std::priority_queue<std::size_t, std::vector<std::size_t>, decltype(sort_crit)> max_heap(idcs.begin(), idcs.end(), sort_crit);
    std::unordered_set<int> disc_idx;
    std::vector<std::size_t> ret;
    std::size_t idx;

    while(!max_heap.empty())
    {
        idx = max_heap.top();

        if(!disc_idx.count(idx) && _isLocalMax(idx))
        {
            ret.emplace_back(idx);

            for(std::size_t i = (idx - feat_ratio_); i <= (idx + feat_ratio_); i++)
                if(i < num_of_bins_)
                    disc_idx.insert(i);
        }

        max_heap.pop();
    }

    return ret;
}
