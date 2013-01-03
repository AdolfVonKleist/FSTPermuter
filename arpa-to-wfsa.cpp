/*
 arpa-to-wfsa.cpp

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
#include "ARPA2WFST.hpp"
using namespace fst;

DEFINE_string( lm,          "",  "Input ARPA format LM." );
DEFINE_string( eps,    "<eps>",  "Epsilon symbols.");
DEFINE_string( sb,       "<s>",  "Sentence begin token.");
DEFINE_string( se,      "</s>",  "Sentence end token.");
DEFINE_string( unk,    "<UNK>",  "Unknown word token.");
DEFINE_bool( use_phi,    true,  "Use phi back-off transitions.");
DEFINE_string( ofile,       "", "Output file for writing. (STDOUT)");

int main( int argc, char* argv[] ){
  string usage = "arpa2wfsa - Transform an ARPA LM into an equivalent WFSA.\n\n Usage: ";
  set_new_handler(FailedNewHandler);
  SetFlags(usage.c_str(), &argc, &argv, false );

  if( FLAGS_lm.compare("")==0 ){
    cout << "You must supply an ARPA format lm to --lm for conversion!" << endl;
    return 0;
  }
    
  cout << "Initializing..." << endl;
  ARPA2WFST* converter = new ARPA2WFST( FLAGS_lm, FLAGS_eps, FLAGS_sb, FLAGS_se, FLAGS_unk );
  cout << "Converting..." << endl;
  converter->arpa_to_wfst( );
  
  if( FLAGS_use_phi == true ){
    cout << "Phi-ifying..." << endl;
    converter->phi_ify( );
  }
  ArcSort(&converter->arpafst, OLabelCompare<StdArc>());
  converter->arpafst.Write(FLAGS_ofile);

  return 1;
}
