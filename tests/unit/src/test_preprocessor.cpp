#include <catch2/catch.hpp>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <filesystem>
#include "preprocessor.hpp"

TEST_CASE("Preprocessor: Sample rate from file", "[Preprocessor][getFileSampleRate]")
{
    std::filesystem::path samples_dir_path = std::filesystem::path(TEST_DATA_DIR);

    SECTION("16 KHz sample")
    {
        REQUIRE(Preprocessor::getFileSampleRate(std::string(samples_dir_path /= "sample_3s_16_khz.wav")) == 16000);
    }

    SECTION("44.1 KHz sample")
    {
        REQUIRE(Preprocessor::getFileSampleRate(std::string(samples_dir_path /= "sample_3s_44_1_khz.wav")) == 44100);
    }

    SECTION("48 KHz sample")
    {
        REQUIRE(Preprocessor::getFileSampleRate(std::string(samples_dir_path /= "sample_3s_48_khz.wav")) == 48000);
    }

    SECTION("96 KHz sample")
    {
        REQUIRE(Preprocessor::getFileSampleRate(std::string(samples_dir_path /= "sample_3s_96_khz.wav")) == 96000);
    }
}

TEST_CASE("Preprocessor: Constructor with default/custom parameters", "[Preprocessor][Constructor]")
{
    SECTION("Constructor called by using audio file")
    {
        const std::string file_path = std::string(std::filesystem::path(TEST_DATA_DIR) /= "sample_3s_44_1_khz.wav");

        SECTION("Constructor with default parameters")
        {
            REQUIRE_NOTHROW(Preprocessor(file_path));
        }

        SECTION("Constructor with custom parameters (downsampling)")
        {
            REQUIRE_NOTHROW(Preprocessor(file_path, 20000, 51));
        }

        SECTION("Constructor with custom parameters (\"same-sampling\")")
        {
            REQUIRE_NOTHROW(Preprocessor(file_path, 44100, 51));
        }

        SECTION("Constructor with custom parameters (upsampling)")
        {
            REQUIRE_NOTHROW(Preprocessor(file_path, 96000, 51));
        }

        SECTION("Constructor with custom parameters (invalid downsampling frequency)")
        {
            REQUIRE_THROWS_AS(Preprocessor(file_path, 0, 100), std::invalid_argument);
        }

        SECTION("Constructor with custom parameters (invalid FIR filter coefficients)")
        {
            REQUIRE_THROWS_AS(Preprocessor(file_path, 20000, 0), std::invalid_argument);
        }
    }

    SECTION("Constructor called by using explicit sampling rate")
    {
        const uint32_t sampling_rate = 44100;
        
        SECTION("Constructor with default parameters")
        {
            REQUIRE_NOTHROW(Preprocessor(sampling_rate));
        }

        SECTION("Constructor with custom parameters (downsampling)")
        {
            REQUIRE_NOTHROW(Preprocessor(sampling_rate, 20000, 51));
        }

        SECTION("Constructor with custom parameters (\"same-sampling\")")
        {
            REQUIRE_NOTHROW(Preprocessor(sampling_rate, 441000, 51));
        }

        SECTION("Constructor with custom parameters (upsampling)")
        {
            REQUIRE_NOTHROW(Preprocessor(sampling_rate, 96000, 51));
        }

        SECTION("Constructor with custom parameters (invalid downsampling frequency)")
        {
            REQUIRE_THROWS_AS(Preprocessor(sampling_rate, 0, 100), std::invalid_argument);
        }

        SECTION("Constructor with custom parameters (invalid FIR filter coefficients)")
        {
            REQUIRE_THROWS_AS(Preprocessor(sampling_rate, 20000, 0), std::invalid_argument);
        }
    }
}

TEST_CASE("Preprocessor: Preprocess file", "[Preprocessor][preprocessData]")
{
    const std::string file_path = std::string(std::filesystem::path(TEST_DATA_DIR) /= "sample_3s_48_khz.wav");

    SECTION("Preprocess with no output")
    {
        SECTION("Preprocess with downsampling")
        {
            Preprocessor preprocessor(file_path, 8000);
            REQUIRE_NOTHROW(preprocessor.preprocessData(file_path));
        }

        SECTION("Preprocess with \"same-sampling\"")
        {
            Preprocessor preprocessor(file_path, 48000);
            REQUIRE_NOTHROW(preprocessor.preprocessData(file_path));
        }

        SECTION("Preprocess with upsampling")
        {
            Preprocessor preprocessor(file_path, 96000);
            REQUIRE_NOTHROW(preprocessor.preprocessData(file_path));
        }

        SECTION("Preprocess with non-existing file")
        {
            Preprocessor preprocessor(file_path, 20000);
            REQUIRE_THROWS_AS(preprocessor.preprocessData("non-existing path"), std::runtime_error);
        }
    }

    SECTION("Preprocess with output file")
    {
        const std::string output_path = std::string(std::filesystem::path(TEST_DATA_DIR) /= "sample_3s_48_khz-test-output.wav");

        SECTION("Preprocess with downsampling")
        {
            Preprocessor preprocessor(file_path, 8000);
            REQUIRE_NOTHROW(preprocessor.preprocessData(file_path, output_path));
            REQUIRE(std::filesystem::exists(output_path));
            REQUIRE(std::filesystem::is_regular_file(output_path));
            REQUIRE(Preprocessor::getFileSampleRate(output_path) == 8000);
        }

        SECTION("Preprocess with \"same-sampling\"")
        {
            Preprocessor preprocessor(file_path, 48000);
            REQUIRE_NOTHROW(preprocessor.preprocessData(file_path, output_path));
            REQUIRE(std::filesystem::exists(output_path));
            REQUIRE(std::filesystem::is_regular_file(output_path));
            REQUIRE(Preprocessor::getFileSampleRate(output_path) == 48000);
        }

        SECTION("Preprocess with upsampling")
        {
            Preprocessor preprocessor(file_path, 96000);
            REQUIRE_NOTHROW(preprocessor.preprocessData(file_path, output_path));
            REQUIRE(std::filesystem::exists(output_path));
            REQUIRE(std::filesystem::is_regular_file(output_path));
            REQUIRE(Preprocessor::getFileSampleRate(output_path) == 96000);
        }

        std::filesystem::remove(output_path);

        SECTION("Preprocess with non-existing input file")
        {
            Preprocessor preprocessor(file_path, 20000);
            REQUIRE_THROWS_AS(preprocessor.preprocessData("non-existing path", output_path), std::runtime_error);
            REQUIRE_FALSE(std::filesystem::exists(output_path));
            REQUIRE_THROWS_AS(Preprocessor::getFileSampleRate(output_path), std::runtime_error);
        }

        SECTION("Preprocess with non-existing output file")
        {
            const std::string& non_existing = "non/existing/path";
            Preprocessor preprocessor(file_path, 20000);
            REQUIRE_THROWS_AS(preprocessor.preprocessData(file_path, non_existing), std::runtime_error);
            REQUIRE_FALSE(std::filesystem::exists(non_existing));
            REQUIRE_THROWS_AS(Preprocessor::getFileSampleRate(non_existing), std::runtime_error);
        }
    }
}
