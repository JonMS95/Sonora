#include <cstddef>
#include <queue>
#include <string>
#include <optional>
#include <iostream>
#include <stdexcept>
#include "audio_indexer.hpp"
#include "audio_matcher.hpp"
#include "scheduler.hpp"
#include "sonora.hpp"
#include "rq_status_enum.hpp"

Sonora::Sonora( const uint32_t downsmp_freq                 ,
                const std::string& db_path                  ,
                const std::size_t fir_coefs                 ,
                const float frame_duration                  ,
                const uint32_t feature_ratio                ,
                const uint8_t window_size                   ,
                const uint8_t peak_number                   ,
                const uint64_t max_index_rqs                ,
                const std::chrono::minutes index_expire_mins,
                const uint64_t max_match_rqs                ,
                const std::chrono::minutes match_expire_mins,
                const uint64_t max_match_threads            ):
    audio_indexer_(AudioIndexer(downsmp_freq    ,
                                db_path         ,
                                fir_coefs       ,
                                frame_duration  ,
                                feature_ratio   ,
                                window_size     ,
                                peak_number     )),
    audio_matcher_(AudioMatcher(downsmp_freq    ,
                                db_path         ,
                                fir_coefs       ,
                                frame_duration  ,
                                feature_ratio   ,
                                window_size     ,
                                peak_number     )),
    index_worker_([this](const std::string& file)
    {
        audio_indexer_.index(file);
        return std::nullopt;
    })                                          ,
    index_saver_([](index_op_info_t&, const std::optional<std::string>&) -> void
    {}                                          ),
    index_scheduler_(   index_worker_           ,
                        index_saver_            ,
                        max_index_rqs           ,
                        index_expire_mins       ,
                        1                       ),
    match_worker_([this](const std::string& file) -> std::string
    {
        return audio_matcher_.match(file);
    }                                           ),
    match_saver_([](match_op_info_t& op, const std::optional<std::string>& ret) -> void
    {
        if(ret.has_value())
            op.ret = ret.value();
    }                                           ),
    match_scheduler_(   match_worker_           ,
                        match_saver_            ,
                        max_match_rqs           ,
                        match_expire_mins       ,
                        max_match_threads       )
{}

Sonora::~Sonora(void)
{
    end();
}

void Sonora::run(void)
{
    index_scheduler_.run();
    match_scheduler_.run();
}

void Sonora::end(void)
{
    index_scheduler_.end();
    match_scheduler_.end();
}

std::optional<uint64_t> Sonora::index(const std::string& file_path)
{
    return index_scheduler_.enqueueJob(file_path);
}

bool Sonora::hasPendingIndexOps(void)
{
    return index_scheduler_.hasPendingOps();
}

request_status_t Sonora::getIndexStatus(const uint64_t job_id)
{
    return index_scheduler_.getJobStatus(job_id);
}

std::optional<uint64_t> Sonora::match(const std::string& file_path)
{
    return match_scheduler_.enqueueJob(file_path);
}

bool Sonora::hasPendingMatchOps(void)
{
    return match_scheduler_.hasPendingOps();
}

request_status_t Sonora::getMatchStatus(const uint64_t job_id)
{
    return match_scheduler_.getJobStatus(job_id);
}

std::string Sonora::getMatchResult(const uint64_t job_id)
{
    return match_scheduler_.getJobResult(job_id).ret;
}
