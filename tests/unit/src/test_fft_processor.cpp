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

TEST_CASE("FFT Processor: computePowerSpectrum", "[FFT Processor][computePowerSpectrum]")
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

        SECTION("Different order leads to same power spectrum")
        {
            frame = std::vector<float>(fft_samples_per_frame);
            std::iota(frame.begin(), frame.end(), 0);
            std::vector<float> fft_ret = fft_proc.computePowerSpectrum(frame);

            SECTION("Reverse")
            {
                for(std::size_t idx = 0; idx < frame.size() / 2; idx++)
                    std::swap(frame[idx], frame[frame.size() - idx - 1]);
                
                REQUIRE(fft_ret == fft_proc.computePowerSpectrum(frame));
            }

            SECTION("Circular shift")
            {
                std::vector<std::vector<float>> rotated_fft_vector;
                for(std::size_t idx = 0; idx < frame.size(); idx++)
                {
                    rotated_fft_vector.emplace_back(fft_proc.computePowerSpectrum(frame));
                    std::rotate(frame.begin(), frame.begin() + 1, frame.end());
                }

                auto check_same_fft = [fft_ret](const std::vector<float>& rotated_fft) -> bool
                {
                    float rel_error;
                    float eps = 1e-5f;

                    for(std::size_t idx = 0; idx < fft_ret.size(); idx++)
                    {
                        rel_error = std::abs(fft_ret[idx] - rotated_fft[idx]) / std::max(std::abs(fft_ret[idx]), std::abs(rotated_fft[idx]));

                        if(rel_error > eps)
                            return false;
                    }
                    return true;
                };

                REQUIRE(std::all_of(rotated_fft_vector.begin(), rotated_fft_vector.end(), check_same_fft));
            }
        }
    }
}

TEST_CASE("FFT Processor: featExt", "[FFT Processor][featExt]")
{
    const float frame_duration = .2f;
    const uint32_t sampling_frequency = 1000;
    const uint32_t feature_ratio = 5;

    FFTProcessor fft_proc(frame_duration, sampling_frequency, feature_ratio); // Samples per frame: 200
    std::vector<float> frame;
    const uint32_t fft_samples_per_frame = frame_duration * sampling_frequency;

    SECTION("Test frame size")
    {
        SECTION("Samples in frame == expected")
        {
            frame.resize(static_cast<std::size_t>(fft_samples_per_frame));
            REQUIRE_NOTHROW(fft_proc.featExt(frame));
        }

        SECTION("Samples in frame > expected")
        {
            frame.resize(static_cast<std::size_t>(fft_samples_per_frame + 1));
            REQUIRE_THROWS_AS(fft_proc.featExt(frame), std::invalid_argument);
        }

        SECTION("Samples in frame < expected")
        {
            frame.resize(static_cast<std::size_t>(fft_samples_per_frame - 1));
            REQUIRE_THROWS_AS(fft_proc.featExt(frame), std::invalid_argument);
        }
    }

    SECTION("Known responses")
    {
        frame.resize(static_cast<std::size_t>(fft_samples_per_frame));
        const std::vector<std::size_t> const_elems_feats = {62, 30, 14, 6, 0, 49, 99, 23, 93, 43, 87, 81, 37, 75, 69, 55};

        SECTION("All zeros")
        {
            frame = std::vector<float>(fft_samples_per_frame, .0f);

            REQUIRE(const_elems_feats == fft_proc.featExt(frame));
        }

        SECTION("Unit impulse")
        {
            frame = std::vector<float>(fft_samples_per_frame, .0f);
            frame[0] = 1;

            REQUIRE(const_elems_feats == fft_proc.featExt(frame));
        }

        SECTION("Constant signal")
        {
            frame = std::vector<float>(fft_samples_per_frame, 2.0f);
            std::vector<std::size_t> expected = {0, 62, 30, 14, 6, 49, 99, 23, 93, 43, 87, 81, 37, 75, 69, 55};

            REQUIRE(expected == fft_proc.featExt(frame));
        }

        SECTION("Cosine signal")
        {
            frame = makeCosine(fft_samples_per_frame, 5);
            std::vector<std::size_t> expected = {5, 26, 62, 41, 68, 76, 56, 85, 96, 47, 16};

            REQUIRE(expected == fft_proc.featExt(frame));            
        }
    }
}
