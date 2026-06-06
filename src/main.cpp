// #include <iostream>
#include <vector>
#include <string>
#include "preprocessor.hpp"
#include "spectral_analyzer.hpp"
#include "fingerprint_generator.hpp"
#include "db_handler.hpp"

int main(int argc, char** argv)
{
    if(argc <= 2)
    {
        // std::cout << "Missing parameters" << std::endl;
        return 1;
    }

    const std::string& input    = std::string(argv[1]);
    const std::string& output   = std::string(argv[2]);
    
    uint32_t downsmp_freq = 8000;

    std::string db_path = std::string("fingerprints.db");

    if(argc > 3)
        downsmp_freq = atoi(argv[3]);
    
    if(argc > 4)
        db_path = std::string(argv[4]);

    const float frame_duration = 0.02;
    const int feat_ratio = 20;

    Preprocessor prep(std::string(input), downsmp_freq);
    std::vector<float> prep_signal = prep.preprocessData(input, output);

    SpectralAnalyzer spectral_analyzer(frame_duration, downsmp_freq, feat_ratio);
    std::vector<std::vector<std::size_t>> features = spectral_analyzer.analyze(prep_signal);

    FingerprintGenerator fp_generator(3, 3);
    std::unordered_map<std::size_t, std::vector<uint32_t>> hashes = fp_generator.genFP(features);

    DBHandler db_handler(db_path);
    db_handler.insertFingerprints(input, hashes);

    return 0;
}