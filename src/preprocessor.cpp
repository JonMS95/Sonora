#include <memory>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <sndfile.h>
#include <samplerate.h>

extern "C"
{
#include <libavutil/channel_layout.h>
}

#include "preprocessor.hpp"

void Preprocessor::avFormatContextDeleter(AVFormatContext* fmt)
{
    avformat_close_input(&fmt);
}

void Preprocessor::avCodecContextDeleter(AVCodecContext* ctx)
{
    avcodec_free_context(&ctx);
}

void Preprocessor::swrContextDeleter(SwrContext* swr)
{
    swr_free(&swr);
}

void Preprocessor::avFrameDeleter(AVFrame* frame)
{
    av_frame_free(&frame);
}

void Preprocessor::avPacketDeleter(AVPacket* pkt)
{
    av_packet_free(&pkt);
}

std::optional<uint32_t> Preprocessor::_getFileSampleRateSndfile(const std::string& file_path)
{
    SF_INFO sf_info{};

    std::unique_ptr<SNDFILE, decltype(&sf_close)> p_file(sf_open(file_path.c_str(), SFM_READ, &sf_info), &sf_close);

    if(!p_file)
        return std::nullopt;

    return static_cast<uint32_t>(sf_info.samplerate);
}

uint32_t Preprocessor::_getFileSampleRateFFmpeg(const std::string& file_path)
{
    AVFormatContext* raw = nullptr;

    if(avformat_open_input(&raw, file_path.c_str(), nullptr, nullptr) < 0)
        throw std::runtime_error("Could not open provided file (" + file_path + ")");

    std::unique_ptr<AVFormatContext, decltype(&Preprocessor::avFormatContextDeleter)> p_av_fmt(raw, &Preprocessor::avFormatContextDeleter);

    if(avformat_find_stream_info(p_av_fmt.get(), nullptr) < 0)
        throw std::runtime_error("Could not find stream info (" + file_path + ")");

    const int audio_stream = av_find_best_stream(p_av_fmt.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if(audio_stream < 0)
        throw std::runtime_error("No audio stream found in file (" + file_path + ")");

    return static_cast<uint32_t>(p_av_fmt->streams[audio_stream]->codecpar->sample_rate);
}

uint32_t Preprocessor::getFileSampleRate(const std::string& file_path)
{
    // std::optional<uint32_t> sndfile_ret = Preprocessor::_getFileSampleRateSndfile(file_path);

    // if(sndfile_ret.has_value())
    //     return sndfile_ret.value();

    return Preprocessor::_getFileSampleRateFFmpeg(file_path);
}

static std::optional<FIRFilter> initFIRFilter(const uint32_t smp_rate, const uint32_t downsmp_freq, const std::size_t fir_coefs)
{
    std::optional<FIRFilter> ret = std::nullopt;

    if(downsmp_freq == 0)
        throw std::invalid_argument("Resampling rate cannot be zero");

    if(smp_rate == 0)
        throw std::invalid_argument("Sampling rate cannot be zero");

    const float cutoff = 0.5 * static_cast<float>(downsmp_freq) / smp_rate;

    if(cutoff > .0f && cutoff < .5f)
        ret.emplace(FIRFilter(cutoff, fir_coefs));

    return ret;
}

Preprocessor::Preprocessor( const std::string& file_path,
                            const uint32_t downsmp_freq ,
                            const std::size_t fir_coefs ):
    fir_filter_(initFIRFilter(Preprocessor::getFileSampleRate(file_path), downsmp_freq, fir_coefs)),
    downsmp_freq_(downsmp_freq)
{}

Preprocessor::Preprocessor( const uint32_t smp_rate     ,
                            const uint32_t downsmp_freq ,
                            const std::size_t fir_coefs ):
    fir_filter_(initFIRFilter(smp_rate, downsmp_freq, fir_coefs)),
    downsmp_freq_(downsmp_freq)
{}

std::optional<std::vector<float>> Preprocessor::_readSndfile(const std::string& file_path)
{
    SF_INFO sf_info{};

    std::unique_ptr<SNDFILE, decltype(&sf_close)> p_file(sf_open(file_path.c_str(), SFM_READ, &sf_info), &sf_close);

    if(!p_file)
        return std::nullopt;

    std::vector<float> ret(static_cast<std::size_t>(sf_info.frames) * static_cast<std::size_t>(sf_info.channels));

    const sf_count_t read_frames = sf_readf_float(p_file.get(), ret.data(), sf_info.frames);

    if(read_frames != sf_info.frames)
        throw std::runtime_error("Could not read all frames from file (" + file_path + ")");

    sound_file_info_.num_of_frames   = static_cast<uint64_t>(sf_info.frames);
    sound_file_info_.num_of_channels = static_cast<uint32_t>(sf_info.channels);
    sound_file_info_.sampling_rate   = static_cast<uint32_t>(sf_info.samplerate);

    return ret;
}

std::vector<float> Preprocessor::_readFFmpeg(const std::string& file_path)
{
    AVFormatContext* raw = nullptr;

    if(avformat_open_input(&raw, file_path.c_str(), nullptr, nullptr) < 0)
        throw std::runtime_error("Could not open provided file (" + file_path + ")");

    std::unique_ptr<AVFormatContext, decltype(&Preprocessor::avFormatContextDeleter)> p_av_fmt(raw, &Preprocessor::avFormatContextDeleter);

    if(avformat_find_stream_info(p_av_fmt.get(), nullptr) < 0)
        throw std::runtime_error("Could not find stream info (" + file_path + ")");

    const int audio_stream = av_find_best_stream(p_av_fmt.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if(audio_stream < 0)
        throw std::runtime_error("No audio stream found in file (" + file_path + ")");

    AVCodecParameters* codecpar = p_av_fmt->streams[audio_stream]->codecpar;
    const AVCodec* decoder = avcodec_find_decoder(codecpar->codec_id);

    if(!decoder)
        throw std::runtime_error("Could not find decoder for file (" + file_path + ")");

    std::unique_ptr<AVCodecContext, decltype(&Preprocessor::avCodecContextDeleter)> p_codec_ctx(avcodec_alloc_context3(decoder), &Preprocessor::avCodecContextDeleter);

    if(!p_codec_ctx)
        throw std::runtime_error("Could not allocate codec context (" + file_path + ")");

    if(avcodec_parameters_to_context(p_codec_ctx.get(), codecpar) < 0)
        throw std::runtime_error("Could not copy codec parameters (" + file_path + ")");

    if(avcodec_open2(p_codec_ctx.get(), decoder, nullptr) < 0)
        throw std::runtime_error("Could not open codec (" + file_path + ")");

#if LIBAVUTIL_VERSION_INT >= AV_VERSION_INT(57, 24, 100)
    if(p_codec_ctx->ch_layout.nb_channels == 0)
        av_channel_layout_default(&p_codec_ctx->ch_layout, p_codec_ctx->ch_layout.nb_channels);

    const int n_channels = p_codec_ctx->ch_layout.nb_channels;

    SwrContext* raw_swr = nullptr;

    if(swr_alloc_set_opts2(&raw_swr,
                            &p_codec_ctx->ch_layout, AV_SAMPLE_FMT_FLT, p_codec_ctx->sample_rate,
                            &p_codec_ctx->ch_layout, p_codec_ctx->sample_fmt, p_codec_ctx->sample_rate,
                            0, nullptr) < 0)
        throw std::runtime_error("Could not allocate resampler context (" + file_path + ")");
#else
    if(!p_codec_ctx->channel_layout)
        p_codec_ctx->channel_layout = av_get_default_channel_layout(p_codec_ctx->channels);

    const int64_t in_ch_layout = p_codec_ctx->channel_layout;
    const int n_channels       = p_codec_ctx->channels;

    SwrContext* raw_swr = swr_alloc_set_opts(nullptr,
                                              in_ch_layout, AV_SAMPLE_FMT_FLT, p_codec_ctx->sample_rate,
                                              in_ch_layout, p_codec_ctx->sample_fmt, p_codec_ctx->sample_rate,
                                              0, nullptr);
#endif

    if(!raw_swr)
        throw std::runtime_error("Could not allocate resampler context (" + file_path + ")");

    std::unique_ptr<SwrContext, decltype(&Preprocessor::swrContextDeleter)> p_swr(raw_swr, &Preprocessor::swrContextDeleter);

    if(swr_init(p_swr.get()) < 0)
        throw std::runtime_error("Could not initialize resampler (" + file_path + ")");

    std::unique_ptr<AVPacket, decltype(&Preprocessor::avPacketDeleter)> p_packet(av_packet_alloc(), &Preprocessor::avPacketDeleter);
    std::unique_ptr<AVFrame, decltype(&Preprocessor::avFrameDeleter)> p_frame(av_frame_alloc(), &Preprocessor::avFrameDeleter);

    if(!p_packet || !p_frame)
        throw std::runtime_error("Could not allocate packet/frame (" + file_path + ")");

    std::vector<float> ret;

    auto appendConverted = [&](const uint8_t** in_data, int in_samples) -> void
    {
        uint8_t* out_buf = nullptr;
        const int out_samples_max = swr_get_out_samples(p_swr.get(), in_samples);

        if(out_samples_max <= 0)
            return;

        if(av_samples_alloc(&out_buf, nullptr, n_channels, out_samples_max, AV_SAMPLE_FMT_FLT, 0) < 0)
            throw std::runtime_error("Could not allocate resample buffer (" + file_path + ")");

        const int converted = swr_convert(p_swr.get(), &out_buf, out_samples_max, in_data, in_samples);

        if(converted < 0)
        {
            av_freep(&out_buf);
            throw std::runtime_error("Error while resampling frame (" + file_path + ")");
        }

        const float* samples = reinterpret_cast<const float*>(out_buf);
        ret.insert(ret.end(), samples, samples + static_cast<std::size_t>(converted) * n_channels);

        av_freep(&out_buf);
    };

    auto drainDecoder = [&](void) -> void
    {
        int rc;

        while((rc = avcodec_receive_frame(p_codec_ctx.get(), p_frame.get())) == 0)
        {
            appendConverted(const_cast<const uint8_t**>(p_frame->data), p_frame->nb_samples);
            av_frame_unref(p_frame.get());
        }

        if(rc != AVERROR(EAGAIN) && rc != AVERROR_EOF)
            throw std::runtime_error("Error while decoding frame (" + file_path + ")");
    };

    while(av_read_frame(p_av_fmt.get(), p_packet.get()) >= 0)
    {
        if(p_packet->stream_index == audio_stream)
        {
            if(avcodec_send_packet(p_codec_ctx.get(), p_packet.get()) < 0)
                throw std::runtime_error("Error sending packet to decoder (" + file_path + ")");

            drainDecoder();
        }

        av_packet_unref(p_packet.get());
    }

    avcodec_send_packet(p_codec_ctx.get(), nullptr);
    drainDecoder();

    appendConverted(nullptr, 0);

    sound_file_info_.num_of_frames   = n_channels ? ret.size() / n_channels : 0;
    sound_file_info_.num_of_channels = static_cast<uint32_t>(n_channels);
    sound_file_info_.sampling_rate   = static_cast<uint32_t>(p_codec_ctx->sample_rate);

    return ret;
}

std::vector<float> Preprocessor::_read(const std::string& file_path)
{
    // std::optional<std::vector<float>> sndfile_ret = _readSndfile(file_path);

    // if(sndfile_ret.has_value())
    //     return std::move(sndfile_ret.value());

    return _readFFmpeg(file_path);
}

std::vector<float> Preprocessor::_mono(const std::vector<float>& signal)
{
    const std::size_t n_samples  = signal.size();
    const std::size_t n_channels = sound_file_info_.num_of_channels;

    if(!n_channels)
        throw std::runtime_error("Number of channels cannot be zero");

    if(n_samples % n_channels)
        throw std::runtime_error("Incomplete frames have been found");

    const std::size_t num_of_frames = n_samples / n_channels;

    std::vector<float> ret(num_of_frames);

    float sum;
    for (std::size_t i = 0; i < num_of_frames; i++)
    {
        sum = 0.0f;

        for (std::size_t c = 0; c < n_channels; c++)
            sum += signal[i * n_channels + c];

        ret[i] = sum / n_channels;
    }

    sound_file_info_.num_of_channels = 1;

    return ret;
}

std::vector<float> Preprocessor::_filter(const std::vector<float>& signal) const
{
    if(fir_filter_ != std::nullopt)
        return fir_filter_->applyFIR(signal);

    return signal;
}

std::vector<float> Preprocessor::_resample(const std::vector<float>& signal)
{
    const double ratio = static_cast<double>(downsmp_freq_) / static_cast<double>(sound_file_info_.sampling_rate);

    SRC_DATA data{};
    data.data_in = signal.data();
    data.input_frames = static_cast<long>(signal.size());
    data.src_ratio = ratio;
    data.end_of_input = 1;

    std::vector<float> ret(static_cast<size_t>(signal.size() * ratio) + 1024);

    data.data_out = ret.data();
    data.output_frames = static_cast<long>(ret.size());

    std::unique_ptr<SRC_STATE, decltype(&src_delete)> state(src_new(SRC_SINC_BEST_QUALITY, 1, nullptr), &src_delete);

    if (!state)
        throw std::runtime_error("Failed to create SRC_STATE");

    int error = src_process(state.get(), &data);
    if (error)
        throw std::runtime_error(src_strerror(error));

    ret.resize(static_cast<size_t>(data.output_frames_gen));

    sound_file_info_.sampling_rate = downsmp_freq_;

    return ret;
}

void Preprocessor::_write(const std::vector<float>& signal, const std::string& output_path)
{
    const int n_channels = static_cast<int>(sound_file_info_.num_of_channels);

    if(!n_channels)
        throw std::runtime_error("Number of channels cannot be zero");

    if(signal.size() % n_channels)
        throw std::runtime_error("Data is not frame-aligned");

    SF_INFO sf_info{};
    sf_info.samplerate = static_cast<int>(sound_file_info_.sampling_rate);
    sf_info.channels   = n_channels;
    sf_info.format     = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    std::unique_ptr<SNDFILE, decltype(&sf_close)> p_file(sf_open(output_path.c_str(), SFM_WRITE, &sf_info), &sf_close);

    if(!p_file)
        throw std::runtime_error("Cannot open output file (" + output_path + ")");

    sf_writef_float(p_file.get(), signal.data(), signal.size() / n_channels);
}

std::vector<float> Preprocessor::preprocessData(const std::string& input_path, const std::string& output_path)
{
    std::lock_guard<std::mutex> lock(prep_mtx_);

    sound_file_info_ = {};

    std::vector<float> ret;
    ret = _read(input_path);
    ret = _mono(ret);

    if(static_cast<uint32_t>(sound_file_info_.sampling_rate) > downsmp_freq_)
        ret = _filter(ret);

    if(static_cast<uint32_t>(sound_file_info_.sampling_rate) != downsmp_freq_)
        ret = _resample(ret);

    if(!output_path.empty())
        _write(ret, output_path);

    return ret;
}
