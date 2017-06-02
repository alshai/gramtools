#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "sdsl/suffix_arrays.hpp"
#include "sdsl/wavelet_trees.hpp"
#include <algorithm>
#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>


using namespace sdsl;
using namespace std;

//make SA sampling density and ISA sampling density customizable
//make void fcn and pass csa by reference? return ii?

csa_wt<wt_int<bit_vector,rank_support_v5<>>,2,16777216> csa_constr(std::string fname, 
								   std::string int_al_fname, 
								   std::string memory_log_fname,
								   std::string csa_file,
								   bool fwd, bool verbose)
{
  //   std::ifstream f(fname);
   std::string prg;
   std::ofstream out(memory_log_fname);
   std::streambuf *coutbuf = std::cout.rdbuf();
   FILE* fp;
   int l=0;


   std::ifstream in(fname, std::ios::in | std::ios::binary);
   if (in)
     {
       //std::string contents;
       in.seekg(0, std::ios::end);
       prg.resize(in.tellg());
       in.seekg(0, std::ios::beg);
       in.read(&prg[0], prg.size());
       in.close();
     }
   else
     {
       cout<<"Problem reading PRG input file"<<endl;
       exit(1);
     }


   //  f>>prg;
   //f.close();

   uint64_t *prg_int=(uint64_t*)malloc(prg.length()*sizeof(uint64_t));
   if (prg_int==NULL)
     {
       exit(1);
     }
   uint32_t i=0;
   uint64_t ii=0;

   while (i<prg.length()) 
     {
       if (isdigit(prg[i])) 
	 {
	   int j=1;
	   while(isdigit(prg[i+1])) 
	     {
	       j++;
	       i++;
	     }
	   auto al_ind=prg.substr(i-j+1,j);
	   //uint64_t l=(uint64_t) stoull(al_ind,NULL,0);
	   l=stoull(al_ind,NULL,0);
	 //uint64_t l=boost::lexical_cast<uint64_t>(al_ind); 
	   prg_int[ii]=l;
	 }
       else 
	 {
	   if (prg[i]=='A' or prg[i]=='a') prg_int[ii]=1;
	   if (prg[i]=='C' or prg[i]=='c') prg_int[ii]=2;
	   if (prg[i]=='G' or prg[i]=='g') prg_int[ii]=3;
	   if (prg[i]=='T' or prg[i]=='t') prg_int[ii]=4;
	 }
       i++;
       ii++;// so ii keeps track of actual base position - it's aware of numbers with more than one digit
     }
   
   csa_wt<wt_int<bit_vector,rank_support_v5<>>,2,16777216> csa;
   
   if (verbose)
     {
       cout<<"PRG size: "<<ii<<endl<<"Alphabet size: "<<l<<endl;
     }

   if (fwd==false) {
     char int_al_fname_rev[50];
     strcpy(int_al_fname_rev,int_al_fname.c_str());
     strcat(int_al_fname_rev,"_rev");
     uint64_t prg_int_rev[ii];
     std::reverse_copy(prg_int,prg_int+ii,prg_int_rev);

     fp=fopen(int_al_fname_rev,"wb");
     fwrite(prg_int_rev,sizeof(uint64_t),ii,fp);
     fclose(fp);
     free(prg_int);
     construct(csa,int_al_fname_rev,8);
   }
   else {
     fp=fopen(int_al_fname.c_str(),"wb");
     fwrite(prg_int,sizeof(uint64_t),ii,fp);
     fclose(fp);
     std::cout.rdbuf(out.rdbuf());   
     free(prg_int);
     memory_monitor::start();
     construct(csa,int_al_fname.c_str(),8);  
     memory_monitor::stop();
     memory_monitor::write_memory_log<HTML_FORMAT>(cout);
 
     std::cout.rdbuf(coutbuf);
     store_to_file(csa,csa_file);
   }

   return(csa);
}
