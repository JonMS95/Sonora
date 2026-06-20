#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>
#include "audio_indexer.hpp"
#include "audio_matcher.hpp"
#include "sonora.hpp"

int main(int argc, char** argv)
{
    /*
    1: index_match -> i/m
    2: input -> file path (either absolute or relative)
    3: db_path -> path to database (either absolute or relative)
    4: downsmp_freq -> int
    5: feature_ratio -> int
    6: feature_ratio -> int
    7: window_size -> int
    8: peak_number -> int
    9: fir_coefs -> int
    */
    
    char index_match = 'i';
    std::string input = "dummy";
    std::string db_path = "dummy";
    uint32_t downsmp_freq = 8000;
    std::size_t fir_coefs = 21;
    float frame_duration = 0.1f; char* end;
    uint32_t feature_ratio = 4;
    uint8_t window_size = 3;
    uint8_t peak_number = 3;

    if(argc >= 1)
    {
        if(argv[1][0] != 'i' && argv[1][0] != 'm')
        {
            std::cerr << "Invalid input for index/match (must be either i or m, no other)." << std::endl;
            return -1;
        }

        index_match = argv[1][0];
    }
        
    if(argc >= 2)
        input = std::string(argv[2]);

    if(argc >= 3)
        db_path = std::string(argv[3]);
    
    if(argc >= 4)
        downsmp_freq = atoi(argv[4]);
    
    if(argc >= 5)
        fir_coefs = atoi(argv[5]);

    if(argc >= 6)
    {
        frame_duration = std::strtof(argv[6], &end);

        if (*end != '\0')
        {
            std::cerr << "Invalid frame duration: " << argv[6] << '\n';
            return -1;
        }
    }

    if(argc >= 7)
        feature_ratio = atoi(argv[7]);
    
    if(argc >= 8)
        window_size = atoi(argv[8]);

    if(argc >= 9)
        peak_number = atoi(argv[9]);

    if(index_match == 'i')
    {
        AudioIndexer audio_indexer(downsmp_freq, db_path, fir_coefs, frame_duration, feature_ratio, window_size, peak_number);
        audio_indexer.index(input);
    }
    else
    {
        AudioMatcher audio_matcher(downsmp_freq, db_path, fir_coefs, frame_duration, feature_ratio, window_size, peak_number);
        std::cout << audio_matcher.match(input) << std::endl;
    }

    Sonora sonora(downsmp_freq, db_path);
    
    return 0;
}
