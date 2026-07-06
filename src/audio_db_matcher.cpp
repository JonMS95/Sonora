#include <stdexcept>
#include <memory>
#include <sqlite3.h>
#include "audio_db_base.hpp"
#include "audio_db_matcher.hpp"

AudioDBMatcher::AudioDBMatcher( const std::string& db_path,
                                const uint32_t downsmp_freq ,
                                const std::size_t fir_coefs ,
                                const float frame_duration  ,
                                const uint32_t feature_ratio,
                                const uint8_t window_size   ,
                                const uint8_t peak_number   ):
    AudioDBBase(db_path)
{
    AudioDBBase::_enableWAL();
    _manageParametersTable( downsmp_freq    ,
                            fir_coefs       ,
                            frame_duration  ,
                            feature_ratio   ,
                            window_size     ,
                            peak_number     );
}

void AudioDBMatcher::_manageParametersTable(const uint32_t downsmp_freq ,
                                            const std::size_t fir_coefs ,
                                            const float frame_duration  ,
                                            const uint32_t feature_ratio,
                                            const uint8_t window_size   ,
                                            const uint8_t peak_number   ) const
{
    if(!AudioDBBase::_parametersTableExists())
        throw std::runtime_error("No parameters table was found");
        
    AudioDBBase::_checkParametersTable( downsmp_freq    ,
                                        fir_coefs       ,
                                        frame_duration  ,
                                        feature_ratio   ,
                                        window_size     ,
                                        peak_number     );
}

std::string AudioDBMatcher::_getSongName(const uint32_t song_id) const
{
    const std::string sql =
        "SELECT song_name "
        "FROM songs "
        "WHERE song_id = ?1;";

    sqlite3_stmt* raw_stmt = nullptr;

    if(sqlite3_prepare_v2(db_, sql.c_str(), -1, &raw_stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error("Failed to prepare select statement");
    
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> p_stmt(raw_stmt, &sqlite3_finalize);

    sqlite3_bind_int(p_stmt.get(), 1, song_id);

    std::string result;

    // Expect at most one row
    if(sqlite3_step(p_stmt.get()) == SQLITE_ROW)
    {
        const unsigned char* text =
            sqlite3_column_text(p_stmt.get(), 0);

        if(text)
            result = reinterpret_cast<const char*>(text);
    }

    return result;
}

std::string AudioDBMatcher::queryHashes(const std::unordered_map<std::size_t, std::vector<uint32_t>>& frame_hashes) const
{
    std::unordered_map<uint32_t, std::unordered_map<int64_t, std::size_t>> votes_per_song;

    const std::string& sql = "SELECT song_id, frame_idx FROM fingerprints WHERE hash = ?;";

    sqlite3_stmt* raw_stmt = nullptr;

    if(sqlite3_prepare_v2(db_, sql.c_str(), -1, &raw_stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error("Failed to prepare insert statement");

    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> p_stmt(raw_stmt, &sqlite3_finalize);

    int64_t cur_idx;
    uint32_t song_id;
    int64_t offset;

    int64_t most_voted_song_id = -1;
    std::size_t max_votes = 0;

    // For every index-hashes pair in the provided map:
    for(auto it = frame_hashes.begin(); it != frame_hashes.end(); it++)
    {
        cur_idx = static_cast<int64_t>(it->first);

        // For every hash associated to the current index:
        for(const uint32_t hash : it->second)
        {
            sqlite3_bind_int(p_stmt.get(), 1, static_cast<uint32_t>(hash));

            // Compute for every match in the database for the current hash.
            while(sqlite3_step(p_stmt.get()) == SQLITE_ROW)
            {
                song_id = sqlite3_column_int(p_stmt.get(), 0);
                offset = static_cast<int64_t>(cur_idx) - static_cast<int64_t>(sqlite3_column_int64(p_stmt.get(), 1));
                
                ++votes_per_song[song_id][offset];

                if(votes_per_song[song_id][offset] > max_votes)
                {
                    max_votes = votes_per_song[song_id][offset];
                    most_voted_song_id = song_id;
                }
            }

            // Reset the query statement.
            sqlite3_reset(p_stmt.get());
            sqlite3_clear_bindings(p_stmt.get());
        }
    }

    return _getSongName(most_voted_song_id);
}
