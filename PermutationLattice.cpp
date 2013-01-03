/*
 PermutationLattice.cpp

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
#include "PermutationLattice.hpp"
        
PermutationLattice::PermutationLattice( ) {
   //Default constructor.
   cout << "Class PermutationLattice requires an input ARPA-format LM..." << endl;
}

PermutationLattice::PermutationLattice( vector<int>* _tokens, bool _make_static ){
  //Evaluate the final cascade?
  make_static = _make_static;
  length      = _tokens->size();

  for( size_t i=0; i<_tokens->size(); i++ ){
    if( counts.find(_tokens->at(i)) == counts.end() )
      counts.insert( pair<int,int>(_tokens->at(i), 1) );
    else
      counts[_tokens->at(i)] += 1;
  }

}

void PermutationLattice::generate_component_fsts( ){
  map<int,int>::iterator it;

  //First build the 'skeleton'
  VectorFst<StdArc> skeleton;
  int i;
  for( i=0; i<length; i++ ){
    skeleton.AddState();
    if( i==0 )
      skeleton.SetStart(0);
    
    for( it=counts.begin(); it != counts.end(); it++ )
      skeleton.AddArc( i, StdArc( (*it).first, (*it).first, 
				  StdArc::Weight::One(), i+1 ) );

  }
  skeleton.AddState();
  skeleton.SetFinal(i, StdArc::Weight::One());
  ArcSort(&skeleton,OLabelCompare<StdArc>());
  component_fsts.push_back( skeleton );

  //Next build the individual components
  for( it=counts.begin(); it != counts.end(); it++ ){
    VectorFst<StdArc> component;
    component.AddState();
    component.SetStart(0);
    int i;
    map<int,int>::iterator jt;
    for( i=0; i<(*it).second; i++ ){
      component.AddState();
      component.AddArc( i, StdArc( (*it).first, (*it).first, 
				   StdArc::Weight::One(), i+1 ) );
      for( jt=counts.begin(); jt != counts.end(); jt++ ){
	if( (*jt).first == (*it).first )
	  continue;
	component.AddArc( i, StdArc( (*jt).first, (*jt).first, 
				     StdArc::Weight::One(), i ) );
      }
    }
    component.SetFinal( i, StdArc::Weight::One() );

    for( jt=counts.begin(); jt != counts.end(); jt++ ){
      if( (*jt).first == (*it).first )
	continue;
      component.AddArc( i, StdArc( (*jt).first, (*jt).first, 
				   StdArc::Weight::One(), i ) );
    }
    ArcSort(&component,OLabelCompare<StdArc>());
    component_fsts.push_back( component );
  }

  return;
}
