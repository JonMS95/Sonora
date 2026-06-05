#ifndef PREPROCESSOR_HPP
#define PREPROCESSOR_HPP

#include <sndfile.h>
#include "fir_filter.hpp"

class Preprocessor
{
private:
    FIRFilter fir_filter_;
    const int downsmp_factor_;
    SF_INFO sf_info_;

    std::vector<float>  _read(const std::string& file_path);
    std::vector<float>  _mono(const std::vector<float>& signal);
    std::vector<float>  _filter(const std::vector<float>& signal) const;
    std::vector<float>  _downsample(const std::vector<float>& signal);
    void                _write(const std::vector<float>& signal, const std::string& s);

public:
    Preprocessor(const std::string& file_path, const float downsmp_freq = 8000.0, const int fir_coefs = 101);
    Preprocessor(const int smp_rate, const float downsmp_freq = 8000.0, const int fir_coefs = 101);

    std::vector<float> preprocessData(const std::string& input_path, const std::string& output_path = "");
};

#endif
