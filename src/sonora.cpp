#include "audio_indexer.hpp"
#include "audio_matcher.hpp"
#include "scheduler.hpp"
#include "sonora.hpp"

struct Sonora::Impl
{
    using op_time_t = std::chrono::steady_clock::time_point;

    // Indexing-side definitions
    typedef struct
    {
        request_status_t status;
        op_time_t expire_time;
    } index_op_info_t;

    // Indexing-side variables
    AudioIndexer audio_indexer;

    // Matching-side definitions
    typedef struct
    {
        request_status_t status;
        op_time_t expire_time;
        std::string ret;
    } match_op_info_t;

    // Matching-side variables
    AudioMatcher audio_matcher;

    std::function<Scheduler<index_op_info_t>::work_fn_sig_t> index_worker;
    std::function<Scheduler<index_op_info_t>::save_fn_sig_t> index_saver;
    Scheduler<index_op_info_t> index_scheduler;
    
    std::function<Scheduler<match_op_info_t>::work_fn_sig_t> match_worker;
    std::function<Scheduler<match_op_info_t>::save_fn_sig_t> match_saver;
    Scheduler<match_op_info_t> match_scheduler;

    Impl(   const uint32_t downsmp_freq                 ,
            const std::string& db_path                  ,
            const std::size_t fir_coefs                 ,
            const float frame_duration                  ,
            const uint32_t feature_ratio                ,
            const uint8_t window_size                   ,
            const uint8_t peak_number                   ,
            const uint64_t max_index_rqs                ,
            const std::chrono::minutes index_expire_mins,
            const uint64_t max_index_threads            ,
            const uint64_t max_match_rqs                ,
            const std::chrono::minutes match_expire_mins,
            const uint64_t max_match_threads            );
};

Sonora::Impl::Impl( const uint32_t downsmp_freq                 ,
                    const std::string& db_path                  ,
                    const std::size_t fir_coefs                 ,
                    const float frame_duration                  ,
                    const uint32_t feature_ratio                ,
                    const uint8_t window_size                   ,
                    const uint8_t peak_number                   ,
                    const uint64_t max_index_rqs                ,
                    const std::chrono::minutes index_expire_mins,
                    const uint64_t max_index_threads            ,
                    const uint64_t max_match_rqs                ,
                    const std::chrono::minutes match_expire_mins,
                    const uint64_t max_match_threads            ):
    audio_indexer(AudioIndexer(downsmp_freq     ,
                                db_path         ,
                                fir_coefs       ,
                                frame_duration  ,
                                feature_ratio   ,
                                window_size     ,
                                peak_number     )),
    audio_matcher(AudioMatcher(downsmp_freq     ,
                                db_path         ,
                                fir_coefs       ,
                                frame_duration  ,
                                feature_ratio   ,
                                window_size     ,
                                peak_number     )),
    index_worker([this](const std::string& file)
    {
        audio_indexer.index(file);
        return std::nullopt;
    })                                          ,
    index_saver([](index_op_info_t&, const std::optional<std::string>&) -> void
    {}                                          ),
    index_scheduler(    index_worker            ,
                        index_saver             ,
                        max_index_rqs           ,
                        index_expire_mins       ,
                        max_index_threads       ),
    match_worker([this](const std::string& file) -> std::string
    {
        return audio_matcher.match(file);
    }                                           ),
    match_saver([](match_op_info_t& op, const std::optional<std::string>& ret) -> void
    {
        if(ret.has_value())
            op.ret = ret.value();
    }                                           ),
    match_scheduler(    match_worker            ,
                        match_saver             ,
                        max_match_rqs           ,
                        match_expire_mins       ,
                        max_match_threads       )
{}

Sonora::Sonora( const uint32_t downsmp_freq                 ,
                const std::string& db_path                  ,
                const std::size_t fir_coefs                 ,
                const float frame_duration                  ,
                const uint32_t feature_ratio                ,
                const uint8_t window_size                   ,
                const uint8_t peak_number                   ,
                const uint64_t max_index_rqs                ,
                const std::chrono::minutes index_expire_mins,
                const uint64_t max_index_threads            ,
                const uint64_t max_match_rqs                ,
                const std::chrono::minutes match_expire_mins,
                const uint64_t max_match_threads            ):
    impl_(std::make_unique<Sonora::Impl>(   downsmp_freq        ,
                                            db_path             ,
                                            fir_coefs           ,
                                            frame_duration      ,
                                            feature_ratio       ,
                                            window_size         ,
                                            peak_number         ,
                                            max_index_rqs       ,
                                            index_expire_mins   ,
                                            max_index_threads   , 
                                            max_match_rqs       ,
                                            match_expire_mins   ,
                                            max_match_threads   ))
{}

Sonora::~Sonora(void)
{
    end();
}

void Sonora::run(void)
{
    impl_->index_scheduler.run();
    impl_->match_scheduler.run();
}

void Sonora::end(void)
{
    impl_->index_scheduler.end();
    impl_->match_scheduler.end();
}

std::optional<uint64_t> Sonora::index(const std::string& file_path)
{
    return impl_->index_scheduler.enqueueJob(file_path);
}

bool Sonora::hasPendingIndexOps(void) const
{
    return impl_->index_scheduler.hasPendingOps();
}

bool Sonora::hasOngoingIndexOps(void) const
{
    return impl_->index_scheduler.hasOngoingOps();
}

bool Sonora::isIndexerRunning(void) const
{
    return impl_->index_scheduler.isSchedulerRunning();
}

request_status_t Sonora::getIndexStatus(const uint64_t job_id)
{
    return impl_->index_scheduler.getJobStatus(job_id);
}

std::optional<uint64_t> Sonora::match(const std::string& file_path)
{
    return impl_->match_scheduler.enqueueJob(file_path);
}

bool Sonora::hasPendingMatchOps(void) const
{
    return impl_->match_scheduler.hasPendingOps();
}

bool Sonora::hasOngoingMatchOps(void) const
{
    return impl_->match_scheduler.hasOngoingOps();
}

bool Sonora::isMatcherRunning(void) const
{
    return impl_->match_scheduler.isSchedulerRunning();
}

request_status_t Sonora::getMatchStatus(const uint64_t job_id)
{
    return impl_->match_scheduler.getJobStatus(job_id);
}

std::string Sonora::getMatchResult(const uint64_t job_id) const
{
    return impl_->match_scheduler.getJobResult(job_id).ret;
}
