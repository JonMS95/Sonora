#ifndef DB_HANDLER
#define DB_HANDLER

#include <string>
#include <sqlite3.h>

class DBHandler
{
private:
    sqlite3* db_;

    void _createSongsTable(void) const;
    void _createFingerprintsTable(void) const;

public:
    DBHandler(const std::string& db_path);
};

#endif
