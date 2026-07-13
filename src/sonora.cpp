#include "audio_indexer.hpp"
#include "audio_matcher.hpp"
#include "scheduler.hpp"
#include "sonora.hpp"

class Sonora::Impl
{
public:
    using op_time_t = std::chrono::steady_clock::time_point;

    // Indexing-side definitions
    typedef struct
    {
        request_status_t status;
        op_time_t expire_time;
    } index_op_info_t;

    // Indexing-side variables
    AudioIndexer audio_indexer_;

    // Matching-side definitions
    typedef struct
    {
        request_status_t status;
        op_time_t expire_time;
        std::string ret;
    } match_op_info_t;

    // Matching-side variables
    AudioMatcher audio_matcher_;

    // Indexing-side functions
    void _indexRoutine(void);

    // Matching-side functions
    void _matchRoutine(void);

    std::function<Scheduler<index_op_info_t>::work_fn_sig_t> index_worker_;
    std::function<Scheduler<index_op_info_t>::save_fn_sig_t> index_saver_;
    Scheduler<index_op_info_t> index_scheduler_;
    
    std::function<Scheduler<match_op_info_t>::work_fn_sig_t> match_worker_;
    std::function<Scheduler<match_op_info_t>::save_fn_sig_t> match_saver_;
    Scheduler<match_op_info_t> match_scheduler_;

    Impl(const uint32_t downsmp_freq                 ,
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
    impl_(std::make_unique<Sonora::Impl>(   downsmp_freq        ,
                                            db_path             ,
                                            fir_coefs           ,
                                            frame_duration      ,
                                            feature_ratio       ,
                                            window_size         ,
                                            peak_number         ,
                                            max_index_rqs       ,
                                            index_expire_mins   ,
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
    impl_->index_scheduler_.run();
    impl_->match_scheduler_.run();
}

void Sonora::end(void)
{
    impl_->index_scheduler_.end();
    impl_->match_scheduler_.end();
}

std::optional<uint64_t> Sonora::index(const std::string& file_path)
{
    return impl_->index_scheduler_.enqueueJob(file_path);
}

bool Sonora::hasPendingIndexOps(void) const
{
    return impl_->index_scheduler_.hasPendingOps();
}

bool Sonora::hasOngoingIndexOps(void) const
{
    return impl_->index_scheduler_.hasOngoingOps();
}

request_status_t Sonora::getIndexStatus(const uint64_t job_id)
{
    return impl_->index_scheduler_.getJobStatus(job_id);
}

std::optional<uint64_t> Sonora::match(const std::string& file_path)
{
    return impl_->match_scheduler_.enqueueJob(file_path);
}

bool Sonora::hasPendingMatchOps(void) const
{
    return impl_->match_scheduler_.hasPendingOps();
}

bool Sonora::hasOngoingMatchOps(void) const
{
    return impl_->match_scheduler_.hasOngoingOps();
}

request_status_t Sonora::getMatchStatus(const uint64_t job_id)
{
    return impl_->match_scheduler_.getJobStatus(job_id);
}

std::string Sonora::getMatchResult(const uint64_t job_id) const
{
    return impl_->match_scheduler_.getJobResult(job_id).ret;
}
