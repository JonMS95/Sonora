#include <catch2/catch.hpp>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <numbers>
#include "fft_processor.hpp"

TEST_CASE("FFT Processor: Constructor with custom/default parameters", "[FFT Processor][Constructor]")
{
    SECTION("Valid custom parameters")
    {
        REQUIRE_NOTHROW(FFTProcessor(.2f, 8000, 5));
    }

    SECTION("Invalid frame length")
    {
        SECTION("Frame length < 0")
        {
            REQUIRE_THROWS_AS(FFTProcessor(-.2f, 8000, 5), std::invalid_argument);
        }

        SECTION("Frame length == 0")
        {
            REQUIRE_THROWS_AS(FFTProcessor(0.0f, 8000, 5), std::invalid_argument);
        }
    }

    SECTION("Invalid sampling frequency")
    {
        REQUIRE_THROWS_AS(FFTProcessor(.2f, 0, 5), std::invalid_argument);
    }

    SECTION("Invalid feature ratio")
    {
        REQUIRE_THROWS_AS(FFTProcessor(.2f, 8000, 0), std::invalid_argument);
    }
}

static inline void requireOnlyPositiveOrNullBins(const std::vector<float>& vec)
{
    REQUIRE(std::all_of(vec.begin(), vec.end(), [](const float& f) -> bool { return f >= 0; }));
}

// Implements cos(2πK/N)
static std::vector<float> makeCosine(const std::size_t N, const std::size_t K)
{
    std::vector<float> ret(N);

    for (std::size_t n = 0; n < N; ++n)
        ret[n] = std::cos(2.0f * std::numbers::pi_v<float> * K * n / static_cast<float>(N));

    return ret;
}

static void allElementsZeroButOne(const std::vector<float>& vec)
{
    const float threshold = 10e-9;
    
    REQUIRE(std::count_if(vec.begin(), vec.end(), [threshold](const float& f) -> bool { return f > threshold; }) == 1);
}

// Check whether scaled_vec == (normal_vec * scale)
static void allElementsAreScaled(std::vector<float> normal_vec, std::vector<float> scaled_vec, const float scale)
{
    REQUIRE(normal_vec.size() == scaled_vec.size());

    auto sqrt_elem = [](const float& f) -> float { return std::sqrt(f); };
    
    std::transform(normal_vec.begin(), normal_vec.end(), normal_vec.begin(), sqrt_elem);
    std::transform(scaled_vec.begin(), scaled_vec.end(), scaled_vec.begin(), sqrt_elem);

    for(float& f : normal_vec)
        f *= scale;

    REQUIRE(normal_vec == scaled_vec);
}

TEST_CASE("FFT Processor: FFT", "[FFT Processor][FFT]")
{
    const float frame_duration = .2f;
    const uint32_t sampling_frequency = 1000;
    const uint32_t feature_ratio = 5;

    FFTProcessor fft_proc(frame_duration, sampling_frequency, feature_ratio); // Samples per frame: 200
    std::vector<float> frame;
    const uint32_t fft_samples_per_frame = frame_duration * sampling_frequency;
    const uint32_t number_of_bins = ((frame_duration * sampling_frequency) / 2) + 1;

    SECTION("Test frame size")
    {
        SECTION("Samples in frame == expected")
        {
            frame.resize(static_cast<std::size_t>(fft_samples_per_frame));
            REQUIRE_NOTHROW(fft_proc.computePowerSpectrum(frame));
        }

        SECTION("Samples in frame > expected")
        {
            frame.resize(static_cast<std::size_t>(fft_samples_per_frame + 1));
            REQUIRE_THROWS_AS(fft_proc.computePowerSpectrum(frame), std::invalid_argument);
        }

        SECTION("Samples in frame < expected")
        {
            frame.resize(static_cast<std::size_t>(fft_samples_per_frame - 1));
            REQUIRE_THROWS_AS(fft_proc.computePowerSpectrum(frame), std::invalid_argument);
        }
    }

    SECTION("Known transforms")
    {
        SECTION("All zeros")
        {
            frame = std::vector<float>(fft_samples_per_frame, .0f);
            std::vector<float> fft_ret = fft_proc.computePowerSpectrum(frame);
            requireOnlyPositiveOrNullBins(fft_ret);
            REQUIRE(fft_ret == std::vector<float>(number_of_bins, .0f));
        }

        SECTION("Unit impulse")
        {
            frame = std::vector<float>(fft_samples_per_frame, .0f);
            frame[0] = 1;
            std::vector<float> fft_ret = fft_proc.computePowerSpectrum(frame);
            requireOnlyPositiveOrNullBins(fft_ret);
            REQUIRE(fft_ret == std::vector<float>(number_of_bins, 1.0f));
        }

        SECTION("Constant signal")
        {
            frame = std::vector<float>(fft_samples_per_frame, 2.0f);
            std::vector<float> fft_ret = fft_proc.computePowerSpectrum(frame);
            requireOnlyPositiveOrNullBins(fft_ret);
            REQUIRE(fft_ret[0] != 4.0f);
            REQUIRE(std::all_of(fft_ret.begin() + 1, fft_ret.end(), [](const float& f) -> bool { return (f >= Approx(.0f)); }));
        }

        SECTION("Cosine signal")
        {
            frame = makeCosine(fft_samples_per_frame, 5);
            std::vector<float> fft_ret = fft_proc.computePowerSpectrum(frame);
            requireOnlyPositiveOrNullBins(fft_ret);
            allElementsZeroButOne(fft_ret);
        }
    }

    SECTION("Basic properties")
    {
        SECTION("Amplitude scaling")
        {
            const float amp = 2.0f;

            frame = std::vector<float>(fft_samples_per_frame, amp);
            std::vector<float> fft_ret_0 = fft_proc.computePowerSpectrum(frame);

            const float scale = 3.0f;

            frame = std::vector<float>(fft_samples_per_frame, scale * amp);
            std::vector<float> fft_ret_1 = fft_proc.computePowerSpectrum(frame);

            allElementsAreScaled(fft_ret_0, fft_ret_1, scale);
        }

        SECTION("Linearity")
        {

        }

        SECTION("Circular shift")
        {

        }
    }
}

// std::vector<float> FFTProcessor::FFT(const std::vector<float>& frame)
// {
//     if(frame.size() != samples_in_frame_)
//         throw std::runtime_error("Frame size mismatch in FFT");

//     std::memcpy(frame_ptr_.get(), frame.data(), sizeof(float) * samples_in_frame_);

//     fftwf_execute(fft_plan_);

//     for (std::size_t k = 0; k < num_of_bins_; k++)
//     {
//         float re = fft_comp_.get()[k][0];
//         float im = fft_comp_.get()[k][1];

//         sq_mags_[k] = (re * re + im * im);
//     }

//     return sq_mags_;
// }
