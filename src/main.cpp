#include <vector>
#include <string>
#include <iostream>
#include "audio_indexer.hpp"
#include "audio_matcher.hpp"

int main(int argc, char** argv)
{
    if(argc <= 2)
    {
        // std::cout << "Missing parameters" << std::endl;
        return 1;
    }

    const std::string& input    = std::string(argv[1]);
    const std::string& output   = std::string(argv[2]);
    
    uint32_t downsmp_freq = 16000;

    std::string db_path = std::string("fingerprints.db");

    if(argc > 3)
        downsmp_freq = atoi(argv[3]);
    
    if(argc > 4)
        db_path = std::string(argv[4]);

    const std::size_t   fir_coefs       = 21;
    const float         frame_duration  = 0.1f;
    const uint32_t      feature_ratio   = 4;
    const uint8_t       window_size     = 3;
    const uint8_t       peak_number     = 3;

    // AudioIndexer audio_indexer(downsmp_freq, db_path, fir_coefs, frame_duration, feature_ratio, window_size, peak_number);
    // audio_indexer.index(input, output);

    AudioMatcher audio_matcher(downsmp_freq, db_path, fir_coefs, frame_duration, feature_ratio, window_size, peak_number);
    std::cout << audio_matcher.match(input) << std::endl;

    return 0;
}
