#include <cstdint>
#include <vector>
#include <unordered_map>
#include "fingerprint_generator.hpp"

FingerprintGenerator::FingerprintGenerator(const int window_size, const int peak_number) :
    win_size_(window_size), peak_num_(peak_number) 
{}

uint32_t FingerprintGenerator::_hash(const int peak_a, const int peak_b, const int idx_a, const int idx_b) const
{
    const uint32_t p_a  = static_cast<uint32_t>(peak_a) << 20;
    const uint32_t p_b  = static_cast<uint32_t>(peak_b) << 8;
    const uint32_t dt   = static_cast<uint32_t>(idx_b - idx_a);

    return (p_a | p_b | dt);
}

// For a couple of given frame feature vectors, generate all possible hashes.
std::vector<uint32_t> FingerprintGenerator::_genFramePairHashes(const std::vector<std::vector<int>>& features, const int idx_a, const int idx_b) const
{
    std::vector<uint32_t> ret;
    const std::vector<int>& frame_a = features[idx_a];
    const std::vector<int>& frame_b = features[idx_b];

    for(int a_feat_idx = 0; a_feat_idx < peak_num_; a_feat_idx++)
        for(int b_feat_idx = 0; b_feat_idx < peak_num_; b_feat_idx++)
            ret.emplace_back(_hash(frame_a[a_feat_idx], frame_b[b_feat_idx], idx_a, idx_b));

    return ret;
}

std::unordered_map<int, std::vector<uint32_t>> FingerprintGenerator::genFP(const std::vector<std::vector<int>>& features) const
{
    std::unordered_map<int, std::vector<uint32_t>> ret;
    const int feat_size = static_cast<int>(features.size());

    for(int f_idx = 0; f_idx < feat_size; f_idx++)
        for(int n_idx = (f_idx + 1); n_idx < std::min((f_idx + 1 + win_size_), feat_size); n_idx++)
            ret[f_idx] = _genFramePairHashes(features, f_idx, n_idx);

    return ret;
}
