#ifndef AUDIO_DB_INDEXER_HPP
#define AUDIO_DB_INDEXER_HPP

#include <string>
#include <vector>
#include <cstddef>
#include <unordered_map>
#include <sqlite3.h>
#include "audio_db_base.hpp"

class AudioDBIndexer : public AudioDBBase
{
private:
    void _createParametersTable(const uint32_t downsmp_freq ,
                                const std::size_t fir_coefs ,
                                const float frame_duration  ,
                                const uint32_t feature_ratio,
                                const uint8_t window_size   ,
                                const uint8_t peak_number   ) const;
    void _manageParametersTable(const uint32_t downsmp_freq ,
                                const std::size_t fir_coefs ,
                                const float frame_duration  ,
                                const uint32_t feature_ratio,
                                const uint8_t window_size   ,
                                const uint8_t peak_number   ) const override;
    void _createSongsTable(void) const;
    void _createFingerprintsTable(void) const;
    uint32_t _getOrCreateSongId(const std::string& song_name) const;
    void _deleteFingerprints(const uint32_t song_id) const;

public:
    explicit AudioDBIndexer(const std::string& db_path  ,
                            const uint32_t downsmp_freq ,
                            const std::size_t fir_coefs ,
                            const float frame_duration  ,
                            const uint32_t feature_ratio,
                            const uint8_t window_size   ,
                            const uint8_t peak_number   );

    void insertFingerprints(const std::string& song_name, const std::unordered_map<std::size_t, std::vector<uint32_t>>& frame_hashes) const;
};

#endif
