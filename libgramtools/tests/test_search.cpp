#include <cctype>

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "gtest/gtest.h"

#include "utils.hpp"
#include "prg.hpp"
#include "kmers.hpp"
#include "search.hpp"


class Search : public ::testing::Test {

protected:
    std::string prg_fpath;

    virtual void SetUp() {
        boost::uuids::uuid uuid = boost::uuids::random_generator()();
        const auto uuid_str = boost::lexical_cast<std::string>(uuid);
        prg_fpath = "./prg_" + uuid_str;
    }

    virtual void TearDown() {
        std::remove(prg_fpath.c_str());
    }

    FM_Index fm_index_from_raw_prg(const std::string &prg_raw) {
        std::vector<uint64_t> prg = encode_prg(prg_raw);
        dump_encoded_prg(prg, prg_fpath);
        FM_Index fm_index;
        // TODO: constructing from memory with sdsl::construct_im appends 0 which corrupts
        sdsl::construct(fm_index, prg_fpath, 8);
        return fm_index;
    }

    PRG_Info generate_prg_info(const std::string &prg_raw) {
        PRG_Info prg_info;
        prg_info.fm_index = fm_index_from_raw_prg(prg_raw);
        prg_info.dna_rank = calculate_ranks(prg_info.fm_index);
        prg_info.allele_mask = generate_allele_mask(prg_raw);
        prg_info.max_alphabet_num = max_alphabet_num(prg_raw);
        return prg_info;
    }

};


/*
raw PRG: gcgctggagtgctgt
F -> first char of SA

i	F	BTW	text	SA
0	0	4	g	0
1	1	3	c	1 3 4 3 2 4 3 4 0
2	2	3	g	2 3 2 4 3 3 1 3 4 3 2 4 3 4 0
3	2	3	c	2 4 3 3 1 3 4 3 2 4 3 4 0
4	2	3	t	2 4 3 4 0
5	3	3	g	3 1 3 4 3 2 4 3 4 0
6	3	0	g	3 2 3 2 4 3 3 1 3 4 3 2 4 3 4 0
7	3	2	a	3 2 4 3 3 1 3 4 3 2 4 3 4 0
8	3	4	g	3 2 4 3 4 0
9	3	4	t	3 3 1 3 4 3 2 4 3 4 0
10	3	4	g	3 4 0
11	3	1	c	3 4 3 2 4 3 4 0
12	4	3	t	4 0
13	4	3	g	4 3 2 4 3 4 0
14	4	2	t	4 3 3 1 3 4 3 2 4 3 4 0
15	4	2	0	4 3 4 0
*/


TEST_F(Search, SingleChar_CorrectSaIntervalReturned) {
    const std::string prg_raw = "gcgctggagtgctgt";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto pattern_char = encode_dna_base('g');

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates search_states = {initial_search_state};

    auto result = search_char_bwd(pattern_char,
                                  search_states,
                                  prg_info);
    SearchStates expected = {
            SearchState {
                    SA_Interval {5, 11},
                    VariantSitePath {}
            }
    };
    EXPECT_EQ(result, expected);
}


TEST_F(Search, TwoConsecutiveChars_CorrectFinalSaIntervalReturned) {
    const std::string prg_raw = "gcgctggagtgctgt";
    const auto prg_info = generate_prg_info(prg_raw);

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates initial_search_states = {initial_search_state};

    const auto first_char = encode_dna_base('g');
    auto first_search_states = search_char_bwd(first_char,
                                               initial_search_states,
                                               prg_info);

    const auto second_char = encode_dna_base('t');
    auto second_search_states = search_char_bwd(second_char,
                                                first_search_states,
                                                prg_info);

    const auto &result = second_search_states;
    SearchStates expected = {
            SearchState {
                    SA_Interval {13, 15},
                    VariantSitePath {}
            }
    };
    EXPECT_EQ(result, expected);
}


TEST_F(Search, SingleCharFreqOneInText_SingleSA) {
    const std::string prg_raw = "gcgctggagtgctgt";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto pattern_char = encode_dna_base('a');

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates search_states = {initial_search_state};

    auto result = search_char_bwd(pattern_char,
                                  search_states,
                                  prg_info);
    SearchStates expected = {
            SearchState {
                    SA_Interval {1, 1},
                    VariantSitePath {}
            }
    };
    EXPECT_EQ(result, expected);
}


TEST_F(Search, TwoConsecutiveChars_SingleSaIntervalEntry) {
    const std::string prg_raw = "gcgctggagtgctgt";
    const auto prg_info = generate_prg_info(prg_raw);

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates initial_search_states = {initial_search_state};

    const auto first_char = encode_dna_base('a');
    auto first_search_states = search_char_bwd(first_char,
                                               initial_search_states,
                                               prg_info);

    const auto second_char = encode_dna_base('g');
    auto second_search_states = search_char_bwd(second_char,
                                                first_search_states,
                                                prg_info);

    const auto &result = second_search_states.front().sa_interval;
    SA_Interval expected{5, 5};
    EXPECT_EQ(result, expected);
}


TEST_F(Search, TwoConsecutiveCharsNoValidSaInterval_NoSearchStatesReturned) {
    const std::string prg_raw = "gcgctggagtgctgt";
    const auto prg_info = generate_prg_info(prg_raw);

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates initial_search_states = {initial_search_state};

    const auto first_char = encode_dna_base('a');
    auto first_search_states = search_char_bwd(first_char,
                                               initial_search_states,
                                               prg_info);

    const auto second_char = encode_dna_base('c');
    const auto &result = search_char_bwd(second_char,
                                         first_search_states,
                                         prg_info);

    SearchStates expected = {};
    EXPECT_EQ(result, expected);
}


/*
PRG: gcgct5c6g6a5agtcct

i   F   BTW text  SA   suffix
0   0   4   3     18     0
1   1   5   2     12     1 3 4 2 2 4 0
2   1   6   3     10     1 5 1 3 4 2 2 4 0
3   2   4   2     15     2 2 4 0
4   2   3   4     1      2 3 2 4 5 2 6 3 6 1 5 1 3 4 2 2 4 0
5   2   2   5     16     2 4 0
6   2   3   2     3      2 4 5 2 6 3 6 1 5 1 3 4 2 2 4 0
7   2   5   6     6      2 6 3 6 1 5 1 3 4 2 2 4 0
8   3   0   3     0      3 2 3 2 4 5 2 6 3 6 1 5 1 3 4 2 2 4 0
9   3   2   6     2      3 2 4 5 2 6 3 6 1 5 1 3 4 2 2 4 0
10  3   1   1     13     3 4 2 2 4 0
11  3   6   5     8      3 6 1 5 1 3 4 2 2 4 0
12  4   2   1     17     4 0
13  4   3   3     14     4 2 2 4 0
14  4   2   4     4      4 5 2 6 3 6 1 5 1 3 4 2 2 4 0
15  5   1   2     11     5 1 3 4 2 2 4 0
16  5   4   2     5      5 2 6 3 6 1 5 1 3 4 2 2 4 0
17  6   3   4     9      6 1 5 1 3 4 2 2 4 0
18  6   2   0     7      6 3 6 1 5 1 3 4 2 2 4 0


a sa interval: 1, 2
sa_preceding_marker_index: 1
marker_char: 5
15, 15

sa_preceding_marker_index: 2
marker_char: 6
17, 17


c sa interval: 3, 7
marker_index: 7
marker_char: 5


g sa interval: 8, 11
marker_index: 11
marker_char: 6


t sa interval: 12, 14
N/A
*/


TEST_F(Search, SingleCharAllele_CorrectSkipToSiteStartBoundaryMarker) {
    const std::string prg_raw = "gcgct5c6g6a5agtcct";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto pattern_char = encode_dna_base('g');

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates search_states = {initial_search_state};

    auto char_search_states = search_char_bwd(pattern_char,
                                              search_states,
                                              prg_info);

    const auto &char_search_state = char_search_states.front();
    const auto &markers_search_states = process_markers_search_state(char_search_state,
                                                                     prg_info);
    const auto &first_markers_search_state = markers_search_states.front();

    const auto &result = first_markers_search_state.sa_interval;
    SA_Interval expected = {16, 16};
    EXPECT_EQ(result, expected);
}


TEST_F(Search, SingleCharAllele_SiteStartBoundarySingleSearchState) {
    const std::string prg_raw = "gcgct5c6g6a5agtcct";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto pattern_char = encode_dna_base('g');

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates search_states = {initial_search_state};

    auto char_search_states = search_char_bwd(pattern_char,
                                              search_states,
                                              prg_info);

    const auto &char_search_state = char_search_states.front();
    const auto &markers_search_states = process_markers_search_state(char_search_state,
                                                                     prg_info);

    const auto &result = markers_search_states.size();
    auto expected = 1;
    EXPECT_EQ(result, expected);
}


TEST_F(Search, FirstAlleleSingleChar_CorrectSkipToSiteStartBoundaryMarker) {
    const std::string prg_raw = "gcgct5c6g6a5agtcct";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto pattern_char = encode_dna_base('c');

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates search_states = {initial_search_state};

    auto char_search_states = search_char_bwd(pattern_char,
                                              search_states,
                                              prg_info);

    const auto &char_search_state = char_search_states.front();
    const auto &markers_search_states = process_markers_search_state(char_search_state,
                                                                     prg_info);
    const auto &first_markers_search_state = markers_search_states.front();

    EXPECT_EQ(markers_search_states.size(), 1);

    auto &result = first_markers_search_state.sa_interval;
    SA_Interval expected = {16, 16};
    EXPECT_EQ(result, expected);
}


TEST_F(Search, CharAfterSiteEndAndAllele_FourDifferentSearchStates) {
    const std::string prg_raw = "gcgct5c6g6a5agtcct";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto pattern_char = encode_dna_base('a');

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates search_states = {initial_search_state};

    auto char_search_states = search_char_bwd(pattern_char,
                                              search_states,
                                              prg_info);

    const auto &char_search_state = char_search_states.front();
    const auto &markers_search_states = process_markers_search_state(char_search_state,
                                                                     prg_info);

    EXPECT_EQ(markers_search_states.size(), 4);
}


TEST_F(Search, GivenBoundaryMarker_GetAlleleMarkerSaInterval) {
    const std::string prg_raw = "gcgct5c6g6a5agtcct";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto boundary_marker = 5;

    auto result = get_allele_marker_sa_interval(boundary_marker, prg_info);
    SA_Interval expected = {17, 18};
    EXPECT_EQ(result, expected);
}


/*
PRG: gcgct5c6g6t5agtcct
i	F	BWT	text	SA	suffix
0	0	4	 3	    18	  0
1	1	5	 2	    12	  1 3 4 2 2 4 0
2	2	4	 3	    15	  2 2 4 0
3	2	3	 2	    1	  2 3 2 4 5 2 6 3 6 4 5 1 3 4 2 2 4 0
4	2	2	 4	    16	  2 4 0
5	2	3	 5	    3	  2 4 5 2 6 3 6 4 5 1 3 4 2 2 4 0
6	2	5	 2	    6	  2 6 3 6 4 5 1 3 4 2 2 4 0
7	3	0	 6	    0	  3 2 3 2 4 5 2 6 3 6 4 5 1 3 4 2 2 4 0
8	3	2	 3	    2	  3 2 4 5 2 6 3 6 4 5 1 3 4 2 2 4 0
9	3	1	 6	    13	  3 4 2 2 4 0
10	3	6	 4	    8	  3 6 4 5 1 3 4 2 2 4 0
11	4	2	 5	    17	  4 0
12	4	3	 1	    14	  4 2 2 4 0
13	4	6	 3	    10	  4 5 1 3 4 2 2 4 0
14	4	2	 4	    4	  4 5 2 6 3 6 4 5 1 3 4 2 2 4 0
15	5	4	 2	    11	  5 1 3 4 2 2 4 0
16	5	4	 2	    5	  5 2 6 3 6 4 5 1 3 4 2 2 4 0
17	6	2	 4	    7	  6 3 6 4 5 1 3 4 2 2 4 0
18	6	3	 0	    9	  6 4 5 1 3 4 2 2 4 0
 */


TEST_F(Search, CharAfterBoundaryEndMarker_ReturnedCorrectMarkerChars) {
    const std::string prg_raw = "gcgct5c6g6t5agtcct";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto pattern_char = encode_dna_base('a');

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates search_states = {initial_search_state};

    auto char_search_states = search_char_bwd(pattern_char,
                                              search_states,
                                              prg_info);

    const auto &char_search_state = char_search_states.front();
    const auto &markers_search_states = process_markers_search_state(char_search_state,
                                                                     prg_info);

    std::unordered_set<uint64_t> result;
    for (const auto &search_state: markers_search_states) {
        auto sa_index = search_state.sa_interval.first;
        auto text_index = prg_info.fm_index[sa_index];
        auto marker_char = prg_info.fm_index.text[text_index];
        result.insert(marker_char);
    }
    std::unordered_set<uint64_t> expected = {6, 6, 5};
    EXPECT_EQ(result, expected);
}


TEST_F(Search, CharAfterBoundaryEndMarker_ReturnedCorrectSaIndexes) {
    const std::string prg_raw = "gcgct5c6g6t5agtcct";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto pattern_char = encode_dna_base('a');

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates search_states = {initial_search_state};

    auto char_search_states = search_char_bwd(pattern_char,
                                              search_states,
                                              prg_info);

    const auto &char_search_state = char_search_states.front();
    const auto &markers_search_states = process_markers_search_state(char_search_state,
                                                                     prg_info);

    std::unordered_set<uint64_t> result;
    for (const auto &search_state: markers_search_states) {
        auto start_sa_index = search_state.sa_interval.first;
        result.insert(start_sa_index);
    }
    std::unordered_set<uint64_t> expected = {15, 17, 18};
    EXPECT_EQ(result, expected);
}


TEST_F(Search, CharAfterBoundaryEndMarker_ReturnedSingleCharSaIntervals) {
    const std::string prg_raw = "gcgct5c6g6t5agtcct";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto pattern_char = encode_dna_base('a');

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates search_states = {initial_search_state};

    auto char_search_states = search_char_bwd(pattern_char,
                                              search_states,
                                              prg_info);

    const auto &char_search_state = char_search_states.front();
    const auto &markers_search_states = process_markers_search_state(char_search_state,
                                                                     prg_info);

    std::vector<uint64_t> result;
    for (const auto &search_state: markers_search_states) {
        auto start_sa_index = search_state.sa_interval.first;
        auto end_sa_index = search_state.sa_interval.first;
        auto num_chars_in_sa_interval = end_sa_index - start_sa_index + 1;
        result.push_back(num_chars_in_sa_interval);
    }
    std::vector<uint64_t> expected = {1, 1, 1};
    EXPECT_EQ(result, expected);
}


TEST_F(Search, CharAfterBoundaryEndMarker_ReturnedSearchStatesHaveCorrectLastVariantSiteAttributes) {
    const std::string prg_raw = "gcgct5c6g6t5agtcct";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto pattern_char = encode_dna_base('a');

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates search_states = {initial_search_state};

    auto char_search_states = search_char_bwd(pattern_char,
                                              search_states,
                                              prg_info);

    const auto &char_search_state = char_search_states.front();
    const auto &markers_search_states = process_markers_search_state(char_search_state,
                                                                     prg_info);

    std::vector<VariantSite> result;
    for (const auto &search_state: markers_search_states)
        result.push_back(search_state.last_variant_site);

    std::vector<VariantSite> expected = {
            {5, Allele {1}},
            {5, Allele {2}},
            {5, Allele {3}},
    };
    EXPECT_EQ(result, expected);
}


TEST_F(Search, CharAfterBoundaryEndMarker_ReturnedSearchStatesHaveCorrectVariantSiteRecordedAttributes) {
    const std::string prg_raw = "gcgct5c6g6t5agtcct";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto pattern_char = encode_dna_base('a');

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates search_states = {initial_search_state};

    auto char_search_states = search_char_bwd(pattern_char,
                                              search_states,
                                              prg_info);

    const auto &char_search_state = char_search_states.front();
    const auto &markers_search_states = process_markers_search_state(char_search_state,
                                                                     prg_info);

    std::vector<bool> result;
    for (const auto &search_state: markers_search_states)
        result.push_back(search_state.variant_site_recorded);

    std::vector<bool> expected = {false, false, false};
    EXPECT_EQ(result, expected);
}

TEST_F(Search, GivenAlleleMarkerSaIndex_ReturnAlleleId) {
    const std::string prg_raw = "gcgct5c6g6t5agtcct";
    const auto prg_info = generate_prg_info(prg_raw);

    uint64_t allele_marker_sa_index = 18;
    auto result = get_allele_id(allele_marker_sa_index,
                                prg_info);
    auto expected = 2;
    EXPECT_EQ(result, expected);
}


TEST_F(Search, ThirdAlleleSingleChar_SkipToSiteStartBoundaryMarker) {
    const std::string prg_raw = "gcgct5c6g6t5agtcct";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto pattern_char = encode_dna_base('t');

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates search_states = {initial_search_state};

    auto char_search_states = search_char_bwd(pattern_char,
                                              search_states,
                                              prg_info);

    const auto &char_search_state = char_search_states.front();
    const auto &markers_search_states = process_markers_search_state(char_search_state,
                                                                     prg_info);
    EXPECT_EQ(markers_search_states.size(), 1);
    auto result = markers_search_states.front();
    SearchState expected = {
            SA_Interval {16, 16},
            VariantSitePath {},
            SearchVariantSiteState::outside_variant_site,
            false,
            VariantSite {5, Allele {3}}
    };
    EXPECT_EQ(result, expected);
}


TEST_F(Search, SecondAlleleSingleChar_SkipToSiteStartBoundaryMarker) {
    const std::string prg_raw = "gcgct5c6g6t5agtcct";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto pattern_char = encode_dna_base('g');

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates search_states = {initial_search_state};

    auto char_search_states = search_char_bwd(pattern_char,
                                              search_states,
                                              prg_info);

    const auto &char_search_state = char_search_states.front();
    const auto &markers_search_states = process_markers_search_state(char_search_state,
                                                                     prg_info);
    EXPECT_EQ(markers_search_states.size(), 1);
    auto result = markers_search_states.front();
    SearchState expected = {
            SA_Interval {16, 16},
            VariantSitePath {},
            SearchVariantSiteState::outside_variant_site,
            false,
            VariantSite {5, Allele {2}}
    };
    EXPECT_EQ(result, expected);
}


TEST_F(Search, FirstAlleleSingleChar_SkipToSiteStartBoundaryMarker) {
    const std::string prg_raw = "gcgct5c6g6t5agtcct";
    const auto prg_info = generate_prg_info(prg_raw);
    const auto pattern_char = encode_dna_base('c');

    SearchState initial_search_state = {
            SA_Interval {0, prg_info.fm_index.size() - 1},
            VariantSitePath {}
    };
    SearchStates search_states = {initial_search_state};

    auto char_search_states = search_char_bwd(pattern_char,
                                              search_states,
                                              prg_info);

    const auto &char_search_state = char_search_states.front();
    const auto &markers_search_states = process_markers_search_state(char_search_state,
                                                                     prg_info);
    EXPECT_EQ(markers_search_states.size(), 1);
    auto result = markers_search_states.front();
    SearchState expected = {
            SA_Interval {16, 16},
            VariantSitePath {},
            SearchVariantSiteState::outside_variant_site,
            false,
            VariantSite {5, Allele {1}}
    };
    EXPECT_EQ(result, expected);
}