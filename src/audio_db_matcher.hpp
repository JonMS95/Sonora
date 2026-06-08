#ifndef AUDIO_DB_MATCHER
#define AUDIO_DB_MATCHER

#include <string>
#include <vector>
#include <cstddef>
#include <unordered_map>
#include "audio_db_base.hpp"

class AudioDBMatcher : public AudioDBBase
{
private:
    std::string _getSongName(const uint32_t song_id) const;

public:
    explicit AudioDBMatcher(const std::string& db_path);

    std::string queryHashes(const std::unordered_map<std::size_t, std::vector<uint32_t>>& frame_hashes) const;
};

#endif