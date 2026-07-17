#include <string>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <unordered_map>

namespace DBHelper
{

bool parametersMatch(   const std::string& db_path      ,
                        const uint32_t downsmp_freq     ,
                        const std::size_t fir_coefs     ,
                        const float frame_duration      ,
                        const uint32_t feature_ratio    ,
                        const uint8_t window_size       ,
                        const uint8_t peak_number       );
bool songExists(const std::string& db_path  ,
                const uint32_t song_id      ,
                const std::string& song_name);
bool fingerprintExists( const std::string& db_path  ,
                        const std::string& hash     ,
                        const uint32_t song_id      ,
                        const std::size_t frame_idx );
bool allFingerprintsExist(  const std::string& db_path                                                  , 
                            const uint32_t song_id                                                      ,
                            const std::unordered_map<std::size_t, std::vector<uint32_t>>& frame_hashes  );

}
