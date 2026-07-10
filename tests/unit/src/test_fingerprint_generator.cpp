#include <catch2/catch.hpp>
#include "fingerprint_generator.hpp"

TEST_CASE("Fingerprint Generator: Constructor with custom parameters", "[Fingerprint Generator][Constructor]")
{
    SECTION("Valid custom parameters")
    {
        REQUIRE_NOTHROW(FingerprintGenerator(5));
    }

    SECTION("Invalid window size")
    {
        REQUIRE_THROWS_AS(FingerprintGenerator(0), std::invalid_argument);
    }
}

TEST_CASE("Fingerprint Generator: genFP", "[Fingerprint Generator][genFP]")
{
    FingerprintGenerator fingerprint_generator(5);

    SECTION("Empty fingerprints map")
    {
        std::vector<std::vector<std::size_t>> empty_features(0);
        std::unordered_map<std::size_t, std::vector<uint32_t>> empty_map = fingerprint_generator.genFP(empty_features);

        REQUIRE(empty_map.size() == 0);
    }

    // SECTION("Known fingerprints")
    // {
    //     std::vector<std::vector<std::size_t>> features(0);
    // }
}

/*
std::unordered_map<std::size_t, std::vector<uint32_t>> FingerprintGenerator::genFP(const std::vector<std::vector<std::size_t>>& features) const
{
    std::unordered_map<std::size_t, std::vector<uint32_t>> ret;
    const std::size_t feat_size = features.size();

    for(std::size_t f_idx = 0; f_idx < feat_size; f_idx++)
    {
        if(features[f_idx].empty())
            continue;

        for(std::size_t n_idx = (f_idx + 1); n_idx < std::min((f_idx + 1 + win_size_), feat_size); n_idx++)
        {
            if(features[n_idx].empty())
                continue;

            ret[f_idx] = _genFramePairHashes(features, f_idx, n_idx);
        }
    }

    return ret;
}
*/
