#ifndef AUDIO_DB_MATCHER
#define AUDIO_DB_MATCHER

#include "audio_db_base.hpp"

class AudioDBMatcher : public AudioDBBase
{
public:
    AudioDBMatcher(const std::string& db_path);
};

#endif