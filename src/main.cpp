#include <vector>
#include <string>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>
#include <optional>
#include <unordered_map>
#include "audio_indexer.hpp"
#include "audio_matcher.hpp"
#include "sonora.hpp"

#include <sqlite3.h>
int indexedRowCount(sqlite3* db)
{
    const char* sql = "SELECT COUNT(*) FROM songs;";
    sqlite3_stmt* stmt = nullptr;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: "
                  << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_ROW)
    {
        std::cerr << "Failed to execute COUNT query." << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    int rowCount = sqlite3_column_int(stmt, 0);

    sqlite3_finalize(stmt);

    return rowCount;
}

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

    // if(index_match == 'i')
    // {
    //     AudioIndexer audio_indexer(downsmp_freq, db_path, fir_coefs, frame_duration, feature_ratio, window_size, peak_number);
    //     audio_indexer.index(input);
    // }
    // else
    // {
    //     AudioMatcher audio_matcher(downsmp_freq, db_path, fir_coefs, frame_duration, feature_ratio, window_size, peak_number);
    //     std::cout << audio_matcher.match(input) << std::endl;
    // }

    Sonora sonora(downsmp_freq, db_path, fir_coefs, frame_duration, feature_ratio, window_size, peak_number);

    sonora.run();

    // if(index_match == 'i')
    // {
        // sonora.index(input);

        std::cout << "Indexing songs..." << std::endl;

        // Use input as dir temporarily

        sqlite3* db;

        if (sqlite3_open("dat/fingerprints.db", &db) != SQLITE_OK)
        {
            std::cerr << "Cannot open database." << std::endl;
            return 1;
        }

        sonora.index(input + "/in/" + "1148__walkerbelm__shirty.wav");
        sonora.index(input + "/in/" + "118171__mikobuntu__9.wav");
        sonora.index(input + "/in/" + "121039__thirsk__140-fx-bass-2.wav");
        sonora.index(input + "/in/" + "167068__k0s__spiffy-spank.wav");
        sonora.index(input + "/in/" + "243853__zuluonedrop__dm_70_drums_rub_a_dub.wav");
        sonora.index(input + "/in/" + "258210__mikobuntu__hoover-glitch2.wav");
        sonora.index(input + "/in/" + "258211__mikobuntu__hoover-glitch.wav");
        sonora.index(input + "/in/" + "365646__caboose3146__horror-loop.wav");
        sonora.index(input + "/in/" + "485869__phonosupf__accordion-melody-23.wav");
        sonora.index(input + "/in/" + "488302__phonosupf__piano-chords-4.wav");
        sonora.index(input + "/in/" + "554517__truscience__ts_galaxyperc.wav");
        sonora.index(input + "/in/" + "577068__qubodup__simple-seamless-music-loop.wav");
        sonora.index(input + "/in/" + "850199__voxbox_502__phat-acidic-synth-melody-140bpm.wav");

        while(indexedRowCount(db) != 13)
            std::this_thread::sleep_for(std::chrono::seconds(1));

        std::unordered_map<std::optional<uint64_t>, std::string> job_ids_to_songs;

        job_ids_to_songs[sonora.match(input + "/parts/" + "1148__walkerbelm__shirty_part_015.wav")] = "1148__walkerbelm__shirty_part_015.wav";
        job_ids_to_songs[sonora.match(input + "/parts/" + "118171__mikobuntu__9_part_015.wav")] = "118171__mikobuntu__9_part_015.wav";
        job_ids_to_songs[sonora.match(input + "/parts/" + "121039__thirsk__140-fx-bass-2_part_015.wav")] = "121039__thirsk__140-fx-bass-2_part_015.wav";
        job_ids_to_songs[sonora.match(input + "/parts/" + "167068__k0s__spiffy-spank_part_015.wav")] = "167068__k0s__spiffy-spank_part_015.wav";
        job_ids_to_songs[sonora.match(input + "/parts/" + "243853__zuluonedrop__dm_70_drums_rub_a_dub_part_015.wav")] = "243853__zuluonedrop__dm_70_drums_rub_a_dub_part_015.wav";
        job_ids_to_songs[sonora.match(input + "/parts/" + "258210__mikobuntu__hoover-glitch2_part_015.wav")] = "258210__mikobuntu__hoover-glitch2_part_015.wav";
        job_ids_to_songs[sonora.match(input + "/parts/" + "258211__mikobuntu__hoover-glitch_part_015.wav")] = "258211__mikobuntu__hoover-glitch_part_015.wav";
        job_ids_to_songs[sonora.match(input + "/parts/" + "365646__caboose3146__horror-loop_part_015.wav")] = "365646__caboose3146__horror-loop_part_015.wav";
        job_ids_to_songs[sonora.match(input + "/parts/" + "485869__phonosupf__accordion-melody-23_part_015.wav")] = "485869__phonosupf__accordion-melody-23_part_015.wav";
        job_ids_to_songs[sonora.match(input + "/parts/" + "488302__phonosupf__piano-chords-4_part_015.wav")] = "488302__phonosupf__piano-chords-4_part_015.wav";
        job_ids_to_songs[sonora.match(input + "/parts/" + "554517__truscience__ts_galaxyperc_part_015.wav")] = "554517__truscience__ts_galaxyperc_part_015.wav";
        job_ids_to_songs[sonora.match(input + "/parts/" + "577068__qubodup__simple-seamless-music-loop_part_015.wav")] = "577068__qubodup__simple-seamless-music-loop_part_015.wav";
        job_ids_to_songs[sonora.match(input + "/parts/" + "850199__voxbox_502__phat-acidic-synth-melody-140bpm_part_015.wav")] = "850199__voxbox_502__phat-acidic-synth-melody-140bpm_part_015.wav";

        bool keep_waiting = true;

        while(keep_waiting)
        {
            int num_of_ok_songs = 0;

            for(auto it = job_ids_to_songs.begin(); it != job_ids_to_songs.end(); it++)
            {
                uint64_t job_id = it->first.has_value() ? it->first.value() : 9999;
            
                if(static_cast<int>(sonora.getMatchStatus(job_id)) != 2)
                    break;
                
                num_of_ok_songs++;
            }

            if(num_of_ok_songs == static_cast<int>(job_ids_to_songs.size()))
               keep_waiting = false;
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // while(job_ids_to_songs.size() < 13)
        //     std::this_thread::sleep_for(std::chrono::seconds(1));

        // std::this_thread::sleep_for(std::chrono::seconds(10));

        std::string str = "";

        for(auto it = job_ids_to_songs.begin(); it != job_ids_to_songs.end(); it++)
        {
            std::string job_id_str = (!it->first.has_value()) ? "None" : std::to_string(it->first.value());

            str = "Job ID: " + job_id_str + ", job status: " + std::to_string(static_cast<int>(sonora.getMatchStatus(it->first.value())));

            if(job_id_str == "None")
            {
                std::cout << str << std::endl;
                continue;
            }

            str = str + ", sample name: " + it->second + ", matched song name: " + sonora.getMatchResult(it->first.value());

            std::cout << str << std::endl;
        }

    // }
    // else if(index_match == 'm')
    //     sonora.index(input);
    // else
    // {
        // std::cerr << "Provide either \'i\' or \'m\' as first input parameter, no other" << std::endl;
        // return -1;
    // }

    sonora.end();

    return 0;
}
