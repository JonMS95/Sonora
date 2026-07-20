#ifndef PREPROCESSOR_HPP
#define PREPROCESSOR_HPP

#include <string>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include "fir_filter.hpp"

class Preprocessor
{
private:
    struct SoundFileInfo
    {
        uint64_t num_of_frames;
        uint32_t num_of_channels;
        uint32_t sampling_rate;
    };

    std::optional<FIRFilter> fir_filter_;
    const uint32_t downsmp_freq_;
    SoundFileInfo sound_file_info_;
    std::mutex prep_mtx_;

    std::optional<std::vector<float>> _readSndfile(const std::string& file_path)        ;
    std::vector<float>  _readFFmpeg(const std::string& file_path)                       ;
    std::vector<float>  _read(const std::string& file_path)                             ;
    std::vector<float>  _mono(const std::vector<float>& signal)                         ;
    std::vector<float>  _filter(const std::vector<float>& signal) const                 ;
    std::vector<float>  _resample(const std::vector<float>& signal)                     ;
    void _write(const std::vector<float>& signal, const std::string& output_path)       ;

    static void avFormatContextDeleter(AVFormatContext* fmt);
    static void avCodecContextDeleter(AVCodecContext* ctx);
    static void swrContextDeleter(SwrContext* swr);
    static void avFrameDeleter(AVFrame* frame);
    static void avPacketDeleter(AVPacket* pkt);

    static std::optional<uint32_t> _getFileSampleRateSndfile(const std::string& file_path);
    static uint32_t                _getFileSampleRateFFmpeg(const std::string& file_path);

public:
    explicit Preprocessor(  const std::string& file_path        ,
                            const uint32_t downsmp_freq = 8000.0,
                            const std::size_t fir_coefs = 101   );
    explicit Preprocessor(  const uint32_t smp_rate             ,
                            const uint32_t downsmp_freq = 8000.0,
                            const std::size_t fir_coefs = 101   );

    std::vector<float> preprocessData(const std::string& input_path, const std::string& output_path = "");

    static uint32_t getFileSampleRate(const std::string& file_path);
};

#endif