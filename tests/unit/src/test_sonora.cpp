#include <catch2/catch.hpp>
// #include <filesystem>
// #include <stdexcept>
// #include <cstddef>
// #include <cstdint>
// #include <string>
// #include <functional>
// #include <chrono>
// #include <optional>
#include "rq_status_enum.hpp"
#include "sonora.hpp"

static const uint32_t downsmp_freq     = 8000   ;
static const std::size_t fir_coefs     = 51     ;
static const float frame_duration      = .01f   ;
static const uint32_t feature_ratio    = 10     ;
static const uint8_t window_size       = 3      ;
static const uint8_t peak_number       = 3      ;

// TEST_CASE("Scheduler: Constructor with custom parameters", "[Scheduler][Constructor]")
// {

// }

//     explicit Sonora(const uint32_t downsmp_freq                                                 ,
//                     const std::string& db_path                                                  ,
//                     const std::size_t fir_coefs                     = 101                       ,
//                     const float frame_duration                      = 0.02f                     ,
//                     const uint32_t feature_ratio                    = 20                        ,
//                     const uint8_t window_size                       = 5                         ,
//                     const uint8_t peak_number                       = 3                         ,
//                     const uint64_t max_index_rqs                    = UINT64_MAX                ,
//                     const std::chrono::minutes index_expire_mins    = std::chrono::minutes(10)  ,
//                     const uint64_t max_match_rqs                    = UINT64_MAX                ,
//                     const std::chrono::minutes match_expire_mins    = std::chrono::minutes(10)  ,
//                     const uint64_t max_match_threads                = 16                        );