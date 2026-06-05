#include <string>
#include <vector>
#include <sndfile.h>
#include "preprocessor.hpp"

static int getFileSampleRate(const std::string& file_path)
{
    SF_INFO sf_info;
    SNDFILE* file = sf_open(file_path.c_str(), SFM_READ, &sf_info);

    if (!file)
        throw std::runtime_error("Provided file (" + file_path + ") was not found");

    sf_close(file);

    return sf_info.samplerate;
}

Preprocessor::Preprocessor(const std::string& file_path, const float downsmp_freq, const int fir_coefs):
    fir_filter_(FIRFilter(0.5 * downsmp_freq / getFileSampleRate(file_path), fir_coefs)),
    downsmp_factor_(static_cast<int>(getFileSampleRate(file_path) / downsmp_freq))
{}

Preprocessor::Preprocessor(const int smp_rate, const float downsmp_freq, const int fir_coefs):
    fir_filter_(FIRFilter(0.5 * downsmp_freq / smp_rate, fir_coefs)),
    downsmp_factor_(static_cast<int>(smp_rate / downsmp_freq))
{}

// #include <iostream>

std::vector<float> Preprocessor::_read(const std::string& file_path)
{
    SNDFILE* file = sf_open(file_path.c_str(), SFM_READ, &sf_info_);

    if (!file)
        throw std::runtime_error("Provided file (" + file_path + ") was not found");

    // std::cout << "Sample rate: "    << sf_info_.samplerate  << "\n";
    // std::cout << "Channels: "       << sf_info_.channels    << "\n";
    // std::cout << "Frames: "         << sf_info_.frames      << "\n";

    std::vector<float> ret(sf_info_.frames * sf_info_.channels);

    sf_readf_float(file, ret.data(), sf_info_.frames);

    sf_close(file);

    return ret;
}

std::vector<float> Preprocessor::_mono(const std::vector<float>& signal)
{
    const int n_samples = static_cast<int>(signal.size());
    const int n_channels = sf_info_.channels;

    if(!n_channels)
        throw std::runtime_error("Number of channels cannot be zero");

    if(n_samples % n_channels)
        throw std::runtime_error("Incomplete frames have been found");

    const int num_of_frames = n_samples / n_channels;

    std::vector<float> ret(num_of_frames);

    for (sf_count_t i = 0; i < num_of_frames; i++)
    {
        float sum = 0.0f;

        for (int c = 0; c < n_channels; c++)
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
    std::vector<float> ret;

    ret.reserve(signal.size() / downsmp_factor_);

    for (std::size_t i = 0; i < signal.size(); i += downsmp_factor_)
        ret.push_back(signal[i]);

    sf_info_.samplerate /= downsmp_factor_;

    return ret;
}

void Preprocessor::_write(const std::vector<float>& signal, const std::string& output_path)
{
    const int n_channels = sf_info_.channels;

    if (signal.size() % n_channels != 0)
        throw std::runtime_error("Data is not frame-aligned");

    sf_info_.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

    SNDFILE* file = sf_open(output_path.c_str(), SFM_WRITE, &sf_info_);

    if (!file)
        throw std::runtime_error("Cannot open output file");

    sf_writef_float(file, signal.data(), signal.size() / n_channels);

    sf_close(file);
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
