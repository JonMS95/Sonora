#include <string>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>
#include <optional>
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

    Sonora sonora(downsmp_freq, db_path, fir_coefs, frame_duration, feature_ratio, window_size, peak_number);

    sonora.run();

    if(index_match == 'i')
    {
        std::optional<uint64_t> job_id = sonora.index(input);

        if(job_id == std::nullopt)
        {
            std::cout << "No valid job id was returned, stopping indexing procedure now..." << std::endl;
            return -2;
        }

        std::cout << "Indexing song: " << input << std::endl;
        
        while(sonora.hasPendingIndexOps() || sonora.hasOngoingIndexOps())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        const int index_status = static_cast<int>(sonora.getIndexStatus(job_id.value()));

        if(index_status != 2)
        {
            std::cout << "Index op status did not go as expected, stopping procedure now..." << std::endl;
            return -3;
        }
    }
    else if(index_match == 'm')
    {
        std::optional<uint64_t> job_id = sonora.match(input);

        if(job_id == std::nullopt)
        {
            std::cout << "No valid job id was returned, stopping matching procedure now..." << std::endl;
            return -2;
        }

        std::cout << "Matching song: " << input << std::endl;

        while(sonora.hasPendingMatchOps() || sonora.hasOngoingMatchOps())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::string str = "";

        const int match_status = static_cast<int>(sonora.getMatchStatus(job_id.value()));

        if(match_status != 2)
        {
            std::cout << "Match op status did not go as expected, stopping procedure now..." << std::endl;
            return -3;
        }

        std::cout << "Match result: " << sonora.getMatchResult(job_id.value()) << std::endl;
    }
    else
    {
        std::cerr << "Provide either \'i\' or \'m\' as first input parameter, no other" << std::endl;
        return -1;
    }

    sonora.end();

    return 0;
}
