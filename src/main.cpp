#include <iostream>
#include <vector>
#include "preprocessor.hpp"
#include "spectral_analyzer.hpp"

int main(int argc, char** argv)
{
    if(argc <= 2)
    {
        std::cout << "Missing parameters" << std::endl;
        return 1;
    }

    const std::string& input    = std::string(argv[1]);
    const std::string& output   = std::string(argv[2]);
    
    float downsmp_freq;
    char* end;

    if(argc > 3)
        downsmp_freq = strtof(argv[3], &end);
    else
        downsmp_freq = 8000.0;

    const float frame_duration = 0.02;
    const int feat_ratio = 5;

    Preprocessor prep(std::string(input), downsmp_freq);
    std::vector<float> prep_signal = prep.preprocessData(input, output);

    SpectralAnalyzer spectral_analyzer(frame_duration, downsmp_freq, feat_ratio);
    spectral_analyzer.analyze(prep_signal);

    return 0;
}