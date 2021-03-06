#include <string>
#include <vector>


#ifndef GRAMTOOLS_PARAMETERS_HPP
#define GRAMTOOLS_PARAMETERS_HPP

namespace gram {

    enum class Commands {
        build,
        quasimap
    };

    struct Parameters {
        std::string gram_dirpath;
        std::string linear_prg_fpath;
        std::string encoded_prg_fpath;
        std::string fm_index_fpath;
        std::string sites_mask_fpath;
        std::string allele_mask_fpath;
        std::string sdsl_memory_log_fpath;

        // kmer index file paths
        std::string kmer_index_fpath;
        std::string kmers_fpath;
        std::string kmers_stats_fpath;
        std::string sa_intervals_fpath;
        std::string paths_fpath;

        uint32_t kmers_size;
        uint32_t max_read_size;
        bool all_kmers_flag;

        // quasimap specific parameters
        std::vector<std::string> reads_fpaths;

        std::string allele_sum_coverage_fpath;
        std::string allele_base_coverage_fpath;
        std::string grouped_allele_counts_fpath;

        uint32_t maximum_threads;
    };

}

#endif //GRAMTOOLS_PARAMETERS_HPP
