#include <catch2/catch.hpp>
#include <stdexcept>
#include <cstdint>
#include <cstddef>
#include "audio_base.hpp"

TEST_CASE("Audio Base: Constructor with default/custom parameters", "[Audio Base][Constructor]")
{
    SECTION("Constructor with default parameters")
    {
        REQUIRE_NOTHROW(AudioBase());
    }

    SECTION("Constructor with custom parameters")
    {
        const uint32_t downsmp_freq     = 8000  ;
        const std::size_t fir_coefs     = 51    ;
        const float frame_duration      = .01f  ;
        const uint32_t feature_ratio    = 5     ;
        const uint8_t window_size       = 4     ;
        const uint8_t peak_number       = 3     ;

        SECTION("Correct custom parameters")
        {
            REQUIRE_NOTHROW(AudioBase(  downsmp_freq    ,
                                        fir_coefs       ,
                                        frame_duration  ,
                                        feature_ratio   ,
                                        window_size     ,
                                        peak_number     ));
        }

        SECTION("Invalid downsampling frequency")
        {
            REQUIRE_THROWS_AS(AudioBase(0               ,
                                        fir_coefs       ,
                                        frame_duration  ,
                                        feature_ratio   ,
                                        window_size     ,
                                        peak_number     ),
                                        std::invalid_argument);
        }

        SECTION("Invalid number of FIR filter coeficients")
        {
            REQUIRE_THROWS_AS(AudioBase(downsmp_freq    ,
                                        0               ,
                                        frame_duration  ,
                                        feature_ratio   ,
                                        window_size     ,
                                        peak_number     ),
                                        std::invalid_argument);
        }

        SECTION("Invalid frame duration")
        {
            REQUIRE_THROWS_AS(AudioBase(downsmp_freq    ,
                                        fir_coefs       ,
                                        0               ,
                                        feature_ratio   ,
                                        window_size     ,
                                        peak_number     ),
                                        std::invalid_argument);
        }

        SECTION("Invalid feature ratio")
        {
            REQUIRE_THROWS_AS(AudioBase(downsmp_freq    ,
                                        fir_coefs       ,
                                        frame_duration  ,
                                        0               ,
                                        window_size     ,
                                        peak_number     ),
                                        std::invalid_argument);
        }
        
        SECTION("Invalid window size")
        {
            REQUIRE_THROWS_AS(AudioBase(downsmp_freq    ,
                                        fir_coefs       ,
                                        frame_duration  ,
                                        feature_ratio   ,
                                        0               ,
                                        peak_number     ),
                                        std::invalid_argument);
        }

        SECTION("Invalid number of feature peaks")
        {
            REQUIRE_THROWS_AS(AudioBase(downsmp_freq    ,
                                        fir_coefs       ,
                                        frame_duration  ,
                                        feature_ratio   ,
                                        window_size     ,
                                        0               ),
                                        std::invalid_argument);
        }
    }
}
