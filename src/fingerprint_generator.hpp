#ifndef FINGERPRINT_GENERATOR_HPP
#define FINGERPRINT_GENERATOR_HPP

#include <vector>
#include <cstdint>
#include <cstddef>
#include <unordered_map>

class FingerprintGenerator
{
private:
    const uint8_t win_size_;
    const uint8_t peak_num_;

    uint32_t _hash( const std::size_t peak_a        ,
                    const std::size_t peak_b        ,
                    const std::size_t idx_a         ,
                    const std::size_t idx_b ) const ;
    std::vector<uint32_t> _genFramePairHashes(  const std::vector<std::vector<std::size_t>>& features,
                                                const std::size_t idx_a,
                                                const std::size_t idx_b) const;

public:
    explicit FingerprintGenerator(const uint8_t window_size, const uint8_t peak_number);

    std::unordered_map<std::size_t, std::vector<uint32_t>> genFP(const std::vector<std::vector<std::size_t>>& features) const;
};

#endif