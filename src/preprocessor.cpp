#include <string>
#include <vector>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <sndfile.h>
#include <samplerate.h>
#include "preprocessor.hpp"

uint32_t Preprocessor::getFileSampleRate(const std::string& file_path)
{
    SF_INFO sf_info{};

    // Create unique_ptr by providing data type and destructor method.
    std::unique_ptr<SNDFILE, decltype(&sf_close)> p_file(sf_open(file_path.c_str(), SFM_READ, &sf_info), &sf_close);

    if (!p_file)
        throw std::runtime_error("Provided file (" + file_path + ") was not found");

    return sf_info.samplerate;
}

static float calcFIRFiterCutoff(const uint32_t smp_rate, const uint32_t downsmp_freq)
{
    return (0.5 * static_cast<float>(downsmp_freq) / smp_rate);
}

static float calcFIRFiterCutoff(const uint32_t downsmp_freq, const std::string& file_path)
{
    return (0.5 * static_cast<float>(downsmp_freq) / Preprocessor::getFileSampleRate(file_path));
}

Preprocessor::Preprocessor( const std::string& file_path,
                            const uint32_t downsmp_freq ,
                            const std::size_t fir_coefs ):
    fir_filter_(FIRFilter(calcFIRFiterCutoff(downsmp_freq, file_path), fir_coefs)),
    downsmp_freq_(downsmp_freq)
{}

Preprocessor::Preprocessor( const uint32_t smp_rate     ,
                            const uint32_t downsmp_freq ,
                            const std::size_t fir_coefs ):
    fir_filter_(FIRFilter(calcFIRFiterCutoff(smp_rate, downsmp_freq), fir_coefs)),
    downsmp_freq_(downsmp_freq)
{}

// #include <iostream>

std::vector<float> Preprocessor::_read(const std::string& file_path)
{
    std::unique_ptr<SNDFILE, decltype(&sf_close)> p_file(sf_open(file_path.c_str(), SFM_READ, &sf_info_), &sf_close);

    if (!p_file)
        throw std::runtime_error("Provided file (" + file_path + ") was not found");

    std::vector<float> ret(sf_info_.frames * sf_info_.channels);

    sf_readf_float(p_file.get(), ret.data(), sf_info_.frames);

    return ret;
}

std::vector<float> Preprocessor::_mono(const std::vector<float>& signal)
{
    const std::size_t n_samples     = signal.size();
    const std::size_t n_channels    = sf_info_.channels;

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

    sf_info_.channels = 1;

    return ret;
}

std::vector<float> Preprocessor::_filter(const std::vector<float>& signal) const
{
    return fir_filter_.applyFIR(signal);
}

std::vector<float> Preprocessor::_downsample(const std::vector<float>& signal)
{
    const double ratio = static_cast<double>(downsmp_freq_) / static_cast<double>(sf_info_.samplerate);

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

    sf_info_.samplerate = downsmp_freq_;

    return ret;
}

void Preprocessor::_write(const std::vector<float>& signal, const std::string& output_path)
{
    const int n_channels = sf_info_.channels;

    if (signal.size() % n_channels != 0)
        throw std::runtime_error("Data is not frame-aligned");

    sf_info_.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    std::unique_ptr<SNDFILE, decltype(&sf_close)> p_file(sf_open(output_path.c_str(), SFM_WRITE, &sf_info_), &sf_close);

    if (!p_file)
        throw std::runtime_error("Cannot open output file");

    sf_writef_float(p_file.get(), signal.data(), signal.size() / n_channels);
}

std::vector<float> Preprocessor::preprocessData(const std::string& input_path, const std::string& output_path)
{
    sf_info_ = {0};

    std::vector<float> raw      = _read(input_path);
    std::vector<float> mono     = _mono(raw);
    std::vector<float> filtered = _filter(mono);
    std::vector<float> ret      = _downsample(filtered);

    if(!output_path.empty())
        _write(ret, output_path);

    return ret;
}
