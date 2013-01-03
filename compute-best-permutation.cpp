/*
 compute-best-permutation.cpp

 Copyright (c) [2012-], Josef Robert Novak
 All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted #provided that the following conditions
   are met:

 * Redistributions of source code must retain the above copyright 
   notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above 
   copyright notice, this list of #conditions and the following 
   disclaimer in the documentation and/or other materials provided 
   with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
 STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/
#include <fst/fstlib.h>
#include "util.hpp"
#include "PermutationLattice.hpp"
#include "FstPathFinder.hpp"
using namespace fst;
typedef PhiMatcher<SortedMatcher<Fst<StdArc> > > PM;

void sent2fsa( vector<int>* tokens, VectorFst<StdArc>* sent){
  /*
    Convert the input word list from a vector of symbol IDs
    to an equivalent Finite-State Acceptor.
  */
  StdArc::StateId st = sent->AddState();
  sent->SetStart(st);
  for( size_t i=0; i<tokens->size(); i++ ){
    st = sent->AddState();
    sent->AddArc( i, 
		  StdArc( 
			 tokens->at(i), 
			 tokens->at(i),
			 StdArc::Weight::One(), 
			 i+1 ) );
  }
  sent->SetFinal( st, StdArc::Weight::One() );
  return;
}

void phi_compose( VectorFst<StdArc>* lm, Fst<StdArc>* sent, VectorFst<StdArc>* result ){
  /*
    Use failure transitions in composition.  This requires that the input LM first be 
    modified to support failure composition.  See the 'arpa-to-wfsa' conversion program
    for details on the approach to doing this.

    In general this produces a more compact composition result because it avoids redundant 
    as well as incorrect paths, which may be generated when treating the backoff transitions
    as standard epsilon transitions.
  */
  ComposeFstOptions<StdArc, PM> opts;
  opts.gc_limit = 0;
  opts.matcher1 = new PM(*lm,   MATCH_OUTPUT, 0);
  opts.matcher2 = new PM(*sent, MATCH_NONE, kNoLabel);

  *result = VectorFst<StdArc>(ComposeFst<StdArc>(*lm, *sent, opts));

  return;
}

VectorFst<StdArc> lattice_heuristic_shortest( VectorFst<StdArc>* lm,
					      vector<int>* tokens, 
					      bool use_phi, int nbest,
					      double threshold ){
  /* 
     Compute the most probable permutation of the input word list.
     This version uses the PermutationLattice class, but modifies
     the composition order.  This permits the application of 
     heuristic pruning of the intermediate lattice.  This is 
     inexact but makes it somewhat tractable to handle some larger
     lists of words. Note: slower for shorter lists!
  */

  PermutationLattice pl( tokens, false );
  pl.generate_component_fsts( );

  //Compose the skeleton FSA with the LM instead
  // of building the complete lattice. 
  VectorFst<StdArc> pfst;
  if( use_phi==true )
    phi_compose( lm, &pl.component_fsts[0], &pfst );
  else
    pfst = VectorFst<StdArc>(ComposeFst<StdArc>(*lm, pl.component_fsts[0]));

  //Now prune the intermediate result using the 
  // threshold parameter value
  Prune(&pfst, threshold);

  //Now apply the remaining enforcement FSAs
  for( size_t i=1; i<pl.component_fsts.size(); i++ )
    pfst = VectorFst<StdArc>(ComposeFst<StdArc>(pfst, pl.component_fsts[i]));

  //Finally compute the shortest path through the result
  //NOTE: depending on the value of the threshold parameter
  //      the final result might NOT include the best permutation
  //      because it may have been pruned out.
  VectorFst<StdArc> shortest;
  ShortestPath(pfst, &shortest, nbest);
  shortest.SetInputSymbols(lm->InputSymbols());
  return shortest;
}


VectorFst<StdArc> lattice_shortest( VectorFst<StdArc>* lm, 
				    vector<int>* tokens, 
				    bool use_phi, int nbest ){
  /*
    Compute the most probable permutation of the input word list.  
    This version uses the PermutationLattice class.  This approach builds up
    a lattices from a series of count-like component FSAs which compactly
    describes the set of permutations implied by the input word list.
    
    This approach is considerably more efficient than a brute-force traversal
    of the set of unique permutations.
  */

  PermutationLattice pl( tokens, false );
  //Generate the counter-like components to build the permutation lattice
  pl.generate_component_fsts( );

  //Now iterate through and build up the lattice-cascade from the components.
  //NOTE: we use lazy/on-demand composition here.  The present implementation
  // still generates the static lattice as a final step prior to composition
  // so there is no real advantage to this.  However the next step up in 
  // efficiency would be to build a standard beam decoder using A* or something.
  // In this case the lazy composition cascade would only be expanded on-demand
  // in response to the heuristic pruning determined by the decoder.  May be 
  // inexact, but will make much longer word-lists tractable.
  vector<ComposeFst<StdArc>* > fsts;
  ComposeFst<StdArc> cascade = ComposeFst<StdArc>(pl.component_fsts[0], 
						  pl.component_fsts[1]);
  fsts.push_back(&cascade);
  for( size_t i=2; i<pl.component_fsts.size(); i++ ){
    if( i==pl.component_fsts.size()-1 )
      pl.component_fsts[i].SetOutputSymbols((SymbolTable*)lm->InputSymbols());
    ComposeFst<StdArc>* fst = new ComposeFst<StdArc>( *fsts[i-2], pl.component_fsts[i] );
    fsts.push_back(fst);
  }
  ComposeFst<StdArc>* final = fsts[fsts.size()-1];

  //Now we've got the cascade.  Compute probability of the various paths
  // through the lattice. Yay!
  VectorFst<StdArc> pfst;
  if( use_phi==true )
    phi_compose( lm, final, &pfst );
  else
    pfst = VectorFst<StdArc>(ComposeFst<StdArc>(*lm, *final));

  VectorFst<StdArc> shortest;
  ShortestPath(pfst, &shortest, nbest);
  shortest.SetInputSymbols(lm->InputSymbols());

  return shortest;
}

VectorFst<StdArc> std_shortest( VectorFst<StdArc>* lm, vector<int>* tokens, bool use_phi ){
  /*
    Compute the most probable permutation of the input word list.
    This version uses a basic feature of the C++ STD library, 'next_permutation'.

    The 'next_permutation' algorithm computes a step-by-step lexicographic sort.
    It represents an efficient brute-force method for iterating through all possible
    permutations of a list or vector.  Duplicates will not be considered.  For example,
    "I <unk> <unk>" would generate just 3 permutations, not 6.

    In order to ensure that *all* permutations are considered however, it is necessary 
    to first sort the input.  Then, the LM is used to compute the probability 
    of each permutation, and the final result is the most likely of all permutations.

    The FSA-driven lattice-based approach is much more efficient.
  */
  StdArc::Weight best_cost = StdArc::Weight::Zero();
  VectorFst<StdArc> best_path; 

  //Make sure we start at the beginning
  sort( tokens->begin(), tokens->end() );

  //Iterate through all permutations
  do{
    VectorFst<StdArc> shortest;
    VectorFst<StdArc> sent;
    //Convert the current permutation into an
    // equivalent FSA
    sent2fsa( tokens, &sent );
    sent.SetOutputSymbols(lm->InputSymbols());

    //Use composition to compute the probability of the
    // current permutation given the input LM
    if( use_phi==true ){
      //In fact using phi_compose with a linear FSA 
      // should produce a unique result.
      phi_compose( lm, &sent, &shortest );
    }else{
      //Standard composition will generate numerous paths because the 
      // back-off transition will be treated as a standard <eps>.
      //It may also, on rare occasions generate erroneous results!
      ShortestPath(VectorFst<StdArc>(ComposeFst<StdArc>(sent,*lm)), &shortest);
    }

    //Now compute the total cost.  We could do this via weight-pushing, 
    // but this manual approach is a little more efficient for this 
    // special case, where the result is a linear WFSA.
    StdArc::Weight cost = StdArc::Weight::One();    
    for( StateIterator<VectorFst<StdArc> > siter(shortest); !siter.Done(); 
	 siter.Next() ){
      StdArc::StateId st = siter.Value();
      for( ArcIterator<VectorFst<StdArc> > aiter(shortest, st); !aiter.Done();
	   aiter.Next() ){
	StdArc arc = aiter.Value();
	cost = Times(cost, arc.weight);
	//We could quit here for a small optimization gain
	// if the partial-path cost exceeds best_cost.
      }      
      if( shortest.Final(st) != StdArc::Weight::Zero() )
	cost = Times(cost, shortest.Final(st));
    }

    //Update the best_cost and best_path results, if necessary.
    if( cost.Value() < best_cost.Value() ){
      best_cost = cost;
      best_path = shortest;
    }

  } while( next_permutation(tokens->begin(), tokens->end()) );

  //The final result should be what we're looking for.
  best_path.SetInputSymbols(lm->InputSymbols());

  return best_path;
} 

void print_path( VectorFst<StdArc>* path ){
  /*
    Print out the path(s) through the result, and corresponding scores.
  */

  set<int> skips;
  skips.insert(0);
  FstPathFinder* p = new FstPathFinder( skips );
  p->extract_all_paths( *path );

  for( size_t i=0; i<p->paths.size(); i++ ){
    cout << p->paths[i].cost << "(log10: " 
	 << ((-1*(p->paths[i].cost.Value()))/log(10.)) << ") ";
    for( size_t j=0; j<p->paths[i].path.size(); j++ ){
      cout << path->InputSymbols()->Find(p->paths[i].path[j]);
      if( j != p->paths[i].path.size()-1 )
        cout << " ";
    }
    cout << endl;
  }

  delete p;

  return;
}


DEFINE_string(     lm,      "",  "Input ARPA format LM.");
DEFINE_string(  input,      "",  "Input list of words.");
DEFINE_string(     sb,   "<s>",  "Sentence begin token.");
DEFINE_string(     se,  "</s>",  "Sentence end token.");
DEFINE_string(    sep,     " ",  "Token separator.");
DEFINE_string(    unk, "<unk>",  "Unknown word token.");
DEFINE_bool(  use_phi,    true,  "Use phi back-off transitions.");
DEFINE_bool(  use_lat,    true,  "Use fst-based lattice approach.");
DEFINE_bool( use_nsec,    true,  "Display timing info in nsecs.");
DEFINE_int32(   nbest,       1,  "Display nbest.  N>1 currently requires --use_lat=true.");
DEFINE_int32( verbose,       1,  "Verbosity level.");
DEFINE_double( thresh,      -1,  "Pruning threshold.");
DEFINE_string(  ofile,      "",  "Output file for writing. (STDOUT)");

int main( int argc, char* argv[] ){
  string usage = "arpa2wfsa - Transform an ARPA LM into an equivalent WFSA.\n\n Usage: ";
  set_new_handler(FailedNewHandler);
  SetFlags(usage.c_str(), &argc, &argv, false );

  if( FLAGS_lm.compare("")==0 ){
    cout << "You must supply an ARPA format lm to --lm for conversion!" << endl;
    return 0;
  }

  if( FLAGS_use_lat==false && FLAGS_nbest>1 ){
    cout << "--nbest with N>1 currently requires --use_lat=true." << endl;
    return 0;
  }

  cerr << "Loading WFSA-format LM..." << endl;
  VectorFst<StdArc>* lm = VectorFst<StdArc>::Read(FLAGS_lm);
  if( !lm ){
    cout << "Failed to open lm: " << FLAGS_lm << endl;
    return 0;
  }
  cerr << "Computing best permutation..." << endl;
  SymbolTable* isyms = (SymbolTable*)lm->InputSymbols();
  //Get the <unk> ID, else just map to <eps>
  int unk_id = isyms->Find(FLAGS_unk) != -1 ? isyms->Find(FLAGS_unk) : 0;
  if( unk_id==0 )
    cerr << "Unknown word token: " << FLAGS_unk << " not found. Using <eps>." << endl;
  //Specify whatever we like for these.
  int sb_id  = isyms->AddSymbol(FLAGS_sb);
  int se_id  = isyms->AddSymbol(FLAGS_se);

  //Tokenize the list of input words.  If present, <s> and </s> will be 
  // removed, as these are implicitly represented by the LM.  Unknown words
  // will be mapped to the <unk> token.
  vector<int> tokens = tokenize_entry( &FLAGS_input, &FLAGS_sep, isyms, 
				       unk_id, sb_id, se_id );
  
  timespec start, end, elapsed;

  start = get_time();
  VectorFst<StdArc> best_path;
  if( FLAGS_use_lat ){
    if( FLAGS_thresh == -1.0 ){
      best_path = lattice_shortest( lm, &tokens, FLAGS_use_phi, FLAGS_nbest );
    }else{
      best_path = lattice_heuristic_shortest( lm, &tokens, FLAGS_use_phi, 
					      FLAGS_nbest, FLAGS_thresh );
    }
  }else{
    best_path = std_shortest( lm, &tokens, FLAGS_use_phi );
  }
  end = get_time();
  elapsed = diff(start, end);
  cout << "Time(" << (FLAGS_use_nsec==true ? "nsec" : "sec") << "): " 
       << (FLAGS_use_nsec ? elapsed.tv_nsec : elapsed.tv_sec) << endl;

  if(  best_path.NumStates()==0 ){
    cout << "No valid paths survived.  Try increasing --thresh value?" << endl;
    return 0;
  }else{
    print_path( &best_path );
  }

  return 1;
}
