#ifndef DB_HANDLER
#define DB_HANDLER

#include <string>
#include <vector>
#include <cstddef>
#include <unordered_map>
#include <sqlite3.h>

class DBHandler
{
private:
    sqlite3* db_;

    void _createSongsTable(void) const;
    void _createFingerprintsTable(void) const;
    uint32_t _getOrCreateSongId(const std::string& song_name) const;

public:
    DBHandler(const std::string& db_path);

    void insertFingerprints(const std::string& song_name, const std::unordered_map<std::size_t, std::vector<uint32_t>>& frame_hashes) const;
};

#endif
