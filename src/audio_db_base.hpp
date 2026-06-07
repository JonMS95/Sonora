#ifndef AUDIO_DB_BASE_HPP
#define AUDIO_DB_BASE_HPP

#include <string>
#include <sqlite3.h>

class AudioDBBase
{
protected:
    sqlite3* db_;

public:
    explicit AudioDBBase(const std::string& db_path);
    virtual ~AudioDBBase(void);
};


#endif
