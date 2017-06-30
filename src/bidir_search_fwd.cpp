#include "sdsl/suffix_arrays.hpp"
#include "sdsl/wavelet_trees.hpp"
#include <cassert>
#include "bwt_search.hpp"
#include <tuple>
#include <cstdint>

using namespace sdsl;

std::vector<uint8_t>::iterator bidir_search_fwd(csa_wt<wt_int<bit_vector,rank_support_v5<>>,2,16777216>& csa_rev,
						uint64_t left, uint64_t right,
						uint64_t left_rev, uint64_t right_rev,
						std::vector<uint8_t>::iterator pat_begin, std::vector<uint8_t>::iterator pat_end,
						std::list<std::pair<uint64_t,uint64_t>>& sa_intervals, 
						std::list<std::pair<uint64_t,uint64_t>>& sa_intervals_rev,
						std::list<std::vector<std::pair<uint32_t, std::vector<int>>>>& sites,
						std::vector<int> &mask_a, uint64_t maxx, bool& first_del,
						bool kmer_precalc_done, std::unordered_map<uint8_t,std::vector<uint64_t>>& rank_all)

//need to swap * with *_rev everywhere
{
	std::list<std::vector<std::pair<uint32_t, std::vector<int>>>>::iterator it_s,it_s_end;
	std::vector<uint8_t>::iterator pat_it=pat_begin;
	std::list<std::pair<uint64_t,uint64_t>>::iterator it, it_rev, it_end, it_rev_end;
	uint8_t c=*pat_begin;
	bool last,ignore;
	uint64_t left_new, right_new, left_rev_new, right_rev_new;
	std::vector<std::pair<uint32_t, std::vector<int>>> empty_pair_vector;
	std::vector<int> allele_empty;

	assert(left<right);
	assert(right<=csa_rev.size());

	if (sa_intervals.empty()) {
		sa_intervals.push_back(std::make_pair(left,right));
		sa_intervals_rev.push_back(std::make_pair(left_rev,right_rev));
		sites.push_back(empty_pair_vector);
	}

	while (pat_it<pat_end && !sa_intervals.empty()) {

		assert(sa_intervals.size()==sa_intervals_rev.size());
		assert(sa_intervals.size()==sites.size());

		it=sa_intervals.begin();
		it_rev=sa_intervals_rev.begin();
		it_s=sites.begin();

		it_end=sa_intervals.end(); // make these constant iterators
		it_rev_end=sa_intervals_rev.end();
		it_s_end=sites.end();

		if ( (pat_it!=pat_begin) or (kmer_precalc_done==true) ) { 
		  for(;it!=it_end && it_rev!=it_rev_end && it_s!=it_s_end; ++it, ++it_rev, ++it_s) {
		    std::vector<std::pair<uint64_t,uint64_t>> res
		      =csa_rev.wavelet_tree.range_search_2d((*it).first, (*it).second-1, 5, maxx).second;
		    //might want to sort res based on pair.second - from some examples it looks like sdsl already does that so res is already sorted 
		    uint32_t prev_num=0;
		    for (auto z=res.begin(),zend=res.end();z!=zend;++z) {
		      uint64_t i=(*z).first;
		      uint32_t num=(*z).second;
		      
		      if ( (num==prev_num)|| (num%2==0 && num==prev_num+1)) ignore=true;
		      else ignore=false;

		      left_new=(*it).first;
		      right_new=(*it).second;
		      
		      //need original [l,r] to for the next loop iterations

		      left_rev_new=(*it_rev).first;
		      right_rev_new=(*it_rev).second;

		      if (num!=prev_num && num%2==1) {
			if (z+1!=zend && num==(*(z+1)).second) {
			  left_new=csa_rev.C[csa_rev.char2comp[num]]; //need to modify left_rev_new as well?
			  right_new=left_new+2;
			}
			else {
			  left_new=i;
			  right_new=i+1;
			}
		      }
		      
		      last=skip(csa_rev,left_new,right_new,left_rev_new,right_rev_new,num,maxx);

		      // how to alternate between forward and backward?
		      if (it==sa_intervals.begin() && first_del==false && !ignore) {
			sa_intervals.push_back(std::make_pair(left_new,right_new));
			sa_intervals_rev.push_back(std::make_pair(left_rev_new,right_rev_new));
			sites.push_back( std::vector<std::pair<uint32_t, std::vector<int>>>(1,get_location(csa_rev,i,num,last,allele_empty,mask_a)));
					 allele_empty.clear();
					 
					 }		    
					 //there will be entries with pair.second empty (corresp to allele) coming from crossing the last marker
					//can delete them here or in top a fcn when calculating coverages
					else {
						if (ignore) {
							if (num%2==0) sites.back().back()=get_location(csa_rev,i,num,last,sites.back().back().second,mask_a);
							//else ?
						}
						else {
							*it=std::make_pair(left_new,right_new);
							*it_rev=std::make_pair(left_rev_new,right_rev_new);
							(*it_s).push_back(get_location(csa_rev,i,num,last,allele_empty,mask_a));
							allele_empty.clear();
						}
					}
					prev_num=num;  
				}
			}
		}

		assert(sa_intervals.size()==sa_intervals_rev.size());
		assert(sa_intervals.size()==sites.size());

		it=sa_intervals.begin();
		it_rev=sa_intervals_rev.begin();	
		it_s=sites.begin();

		while (it!=sa_intervals.end() && it_rev!=sa_intervals_rev.end() && it_s!=sites.end()) {	
			//calculate sum to return- can do this in top fcns
		  if (bidir_search(c, it, it_rev,csa_rev, rank_all)>0) {
				++it;
				++it_rev;
				++it_s;
			}
			else {
				if (it==sa_intervals.begin()) first_del=true;
				//might need to see first_del from top fcns to check if there are matches in the reference
				it=sa_intervals.erase(it);
				it_rev=sa_intervals_rev.erase(it_rev);
				it_s=sites.erase(it_s);
			}
		}
		++pat_it;
		c=*pat_it;
	}  

	if (pat_it!=pat_begin) return(pat_it); // where it got stuck
	else {
		if (!sa_intervals.empty()) return(pat_end);
		else return(pat_begin); //where it got stuck
	}

}			     			     			          
