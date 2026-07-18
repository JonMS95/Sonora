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
    FingerprintGenerator fingerprint_generator(3);

    SECTION("Empty fingerprints map")
    {
        std::vector<std::vector<std::size_t>> empty_features(0);
        std::unordered_map<std::size_t, std::vector<uint32_t>> empty_map = fingerprint_generator.genFP(empty_features);

        REQUIRE(empty_map.size() == 0);
    }

    SECTION("Known fingerprint")
    {
        std::vector<std::vector<std::size_t>> features =
        {   {69 ,   95  ,   63  },
            {86 ,   25  ,   15  },
            {25 ,   69  ,   55  },
            {19 ,   39  ,   70  },
            {0  ,   44  ,   50  },
        };

        std::unordered_map<std::size_t, std::vector<uint32_t>> fp_map = fingerprint_generator.genFP(features);
        
        REQUIRE(fp_map.size() == static_cast<std::size_t>(features.size() - 1));
        std::unordered_map<std::size_t, std::vector<uint32_t>> expected_fp_map =
        {
            {
                0,
                {
                    72373761, 72358145, 72355585,
                    99636737, 99621121, 99618561,
                    66082305, 66066689, 66064129,
                    72358146, 72369410, 72365826,
                    99621122, 99632386, 99628802,
                    66066690, 66077954, 66074370,
                    72356611, 72361731, 72369667,
                    99619587, 99624707, 99632643,
                    66065155, 66070275, 66078211,
                }
            },
            {
                1,
                {
                    90183937, 90195201, 90191617,
                    26220801, 26232065, 26228481,
                    15735041, 15746305, 15742721,
                    90182402, 90187522, 90195458,
                    26219266, 26224386, 26232322,
                    15733506, 15738626, 15746562,
                    90177539, 90188803, 90190339,
                    26214403, 26225667, 26227203,
                    15728643, 15739907, 15741443,
                }
            },
            {
                2,
                {
                    26219265, 26224385, 26232321,
                    72356609, 72361729, 72369665,
                    57676545, 57681665, 57689601,
                    26214402, 26225666, 26227202,
                    72351746, 72363010, 72364546,
                    57671682, 57682946, 57684482,
                }                
            },
            {
                3,
                {
                    19922945, 19934209, 19935745,
                    40894465, 40905729, 40907265,
                    73400321, 73411585, 73413121,
                }
            },
        };

        REQUIRE(fp_map == expected_fp_map);
    }
}
