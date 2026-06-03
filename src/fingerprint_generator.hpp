#ifndef FINGERPRINT_GENERATOR_HPP
#define FINGERPRINT_GENERATOR_HPP

#include <vector>
#include <cstdint>
#include <unordered_map>

class FingerprintGenerator
{
private:
    const int win_size_;

    uint32_t _hash(const int peak_a, const int peak_b, const int idx_a, const int idx_b) const;
    std::vector<uint32_t> _genFramePairHashes(const std::vector<std::vector<int>>& features, const int idx_a, const int idx_b) const;

public:
    FingerprintGenerator(const int window_size);

    std::unordered_map<int, std::vector<uint32_t>> genFP(const std::vector<std::vector<int>>& features) const;
};

#endif