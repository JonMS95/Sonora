#ifndef SONORA_HPP
#define SONORA_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <chrono>
#include <optional>
#include <memory>
#include "rq_status_enum.hpp"

// Should we make this a singleton??
class Sonora
{
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

public:
    explicit Sonora(const uint32_t downsmp_freq                                                 ,
                    const std::string& db_path                                                  ,
                    const std::size_t fir_coefs                     = 101                       ,
                    const float frame_duration                      = 0.02f                     ,
                    const uint32_t feature_ratio                    = 20                        ,
                    const uint8_t window_size                       = 5                         ,
                    const uint8_t peak_number                       = 3                         ,
                    const uint64_t max_index_rqs                    = UINT64_MAX                ,
                    const std::chrono::minutes index_expire_mins    = std::chrono::minutes(10)  ,
                    const uint64_t max_match_rqs                    = UINT64_MAX                ,
                    const std::chrono::minutes match_expire_mins    = std::chrono::minutes(10)  ,
                    const uint64_t max_match_threads                = 16                        );
                    
    virtual ~Sonora(void);                

    void run(void);
    void end(void);

    std::optional<uint64_t> index(const std::string& file_path);
    bool hasPendingIndexOps(void) const;
    bool hasOngoingIndexOps(void) const;
    bool isIndexerRunning(void) const;
    request_status_t getIndexStatus(const uint64_t job_id);

    std::optional<uint64_t> match(const std::string& file_path);
    bool hasPendingMatchOps(void) const;
    bool hasOngoingMatchOps(void) const;
    bool isMatcherRunning(void) const;
    request_status_t getMatchStatus(const uint64_t job_id);
    std::string getMatchResult(const uint64_t job_id) const;
};

#endif