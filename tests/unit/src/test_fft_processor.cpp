#include <catch2/catch.hpp>
#include <cstddef>
#include <cstdint>
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

static void requireOnlyPositiveOrNullBins(const std::vector<float>& bins)
{
    for(std::size_t idx = 0; idx < bins.size(); idx++)
        REQUIRE(bins[idx] >= .0f);
}

TEST_CASE("FFT Processor: FFT", "[FFT Processor][FFT]")
{
    const float frame_duration = .2f;
    const uint32_t sampling_frequency = 1000;
    const uint32_t feature_ratio = 5;

    FFTProcessor fft_proc(frame_duration, sampling_frequency, feature_ratio); // Samples per frame: 200
    std::vector<float> frame;
    const uint32_t fft_samples_per_frame = frame_duration * sampling_frequency;
    const std::size_t samples_per_frame = static_cast<std::size_t>(fft_samples_per_frame);

    SECTION("Test frame size")
    {
        SECTION("Samples in frame == expected")
        {
            frame.resize(static_cast<std::size_t>(fft_samples_per_frame));
            REQUIRE_NOTHROW(fft_proc.FFT(frame));
        }

        SECTION("Samples in frame > expected")
        {
            frame.resize(static_cast<std::size_t>(fft_samples_per_frame + 1));
            REQUIRE_THROWS_AS(fft_proc.FFT(frame), std::invalid_argument);
        }

        SECTION("Samples in frame < expected")
        {
            frame.resize(static_cast<std::size_t>(fft_samples_per_frame - 1));
            REQUIRE_THROWS_AS(fft_proc.FFT(frame), std::invalid_argument);
        }
    }

    SECTION("Known transforms")
    {
        SECTION("All zeros")
        {
            frame = std::vector<float>(fft_samples_per_frame, .0f);
            std::vector<float> fft_ret = fft_proc.FFT(frame);
            requireOnlyPositiveOrNullBins(fft_ret);
            REQUIRE(frame == fft_proc.FFT(frame));
        }

        SECTION("Unit impulse")
        {
            frame = std::vector<float>(fft_samples_per_frame, .0f);
            frame[0] = 1;
            std::vector<float> fft_ret = fft_proc.FFT(frame);
            requireOnlyPositiveOrNullBins(fft_ret);
            REQUIRE(fft_ret == std::vector<float>(fft_samples_per_frame, 1.0f));
        }

        SECTION("Constant signal")
        {
            frame = std::vector<float>(fft_samples_per_frame, 2.0f);
            std::vector<float> fft_ret = fft_proc.FFT(frame);
            requireOnlyPositiveOrNullBins(fft_ret);
            REQUIRE(fft_ret[0] == 4.0f);

            for(std::size_t idx = 1; idx < fft_ret.size(); idx++)
                REQUIRE(fft_ret[idx] == Approx(.0f));
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
