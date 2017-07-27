#include "fm_index.hpp"


#ifndef GRAMTOOLS_RANKS_HPP
#define GRAMTOOLS_RANKS_HPP


using DNA_Rank = std::unordered_map<uint8_t, std::vector<uint64_t>>;

DNA_Rank calculate_ranks(const FM_Index &fm_index);


#endif //GRAMTOOLS_RANKS_HPP