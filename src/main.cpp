// #include <iostream>
#include <vector>
#include <string>
// #include "preprocessor.hpp"
// #include "spectral_analyzer.hpp"
// #include "fingerprint_generator.hpp"
// #include "db_handler.hpp"
#include "audio_indexer.hpp"

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

    AudioIndexer audio_indexer(downsmp_freq, db_path);
    audio_indexer.index(input, output);

    return 0;
}