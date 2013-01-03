FSTPermuter
===========
Josef Robert Novak - 2013-01-01

This distribution provides several small standalone tools suitable 
for computing the most likely sequence(s) of words given an input 
LM and unordered list of input words. The WFST-framework and 
OpenFst library are used as the basis for the tools.

For example, given the list `goes`, `he`, `the`, `park`, `to`, the 
tool will compute the most likely sequence, `he goes to the park` and
the corresponding log probability of the sequence.  n-best results are
also supported for certain special cases.

Scripts for computing the log probability of individual sequences via
OpenGRM or SRILM are also provided for verification purposes.

Requirements
------------
 * [OpenFst library](http://www.openfst.org) -- Make sure to compile the extensions: `./configure --enable-compact-fsts --enable-const-fsts --enable-far --enable-lookahead-fsts --enable-pdt`
 * g++ or comparable compiler
 * OSX or Linux operating

Optional
--------
 * [OpenGRM](http://www.openfst.org/twiki/bin/view/GRM/NGramLibrary) -- For running OpenGRM comparison
 * [python 2.7+](http://www.python.org/getit/) -- For running OpenGRM comparison

Compilation
-----------
Just run `make` from the parent directory. Remember OpenFst has to be installed.
`$ make`

Tools
-----
The distribution includes several tools.
 * `arpa-to-wfsa`: Convert an ARPA format LM to an equivalent WFSA.
 * `compute-best-permutation`: Compute the most probable permutation for an input word list, given an input LM.
 * `get-syms`: Convenience utility for extracting symbol tables from an FST. Useful for OpenGRM verification.
 * `grm-perp.py`: python script that uses OpenGRM to compute the perplexity of an input sentence.  Useful for verification purposes.
 * `srilm.sh`: Example SRILM command to compute the perplexity of a sentence with the SRILM ngram tool.

Examples
--------
The tools provide a range of options; below are several simplest case examples.

### Convert 
Convert an ARPA format LM to WFSA format, using failure transitions.
`$ ./arpa-to-wfsa --lm=testlm.arpa --ofile=testlm.fst`

### Compute - std-based
Compute the most likely permutation of a list of input words given an LM in WFSA format.  This approach uses the C++ STD library function `next_permutation`.  This is not particularly fast.  Note that OOV words will be mapped to the <unk> token.
`$ ./compute-best-permutation --lm=testlm.fst --input="he goes to the jibberish park" --use_lat=false`

### Compute - lattice-based
Compute the most likely permutation of a list of input words given an LM in WFSA format.  Uses the lattice-based method by default, and composition with failure transitions. Note that OOV words will be mapped to the <unk> token.
`$ ./compute-best-permutation --lm=testlm.fst --input="he goes to the jibberish park"`

#### Compute n-best
The lattice-based approach also supports n-best results.

    $ ./compute-best-permutation --lm=testlm.fst --input="he goes to the jibberish park" --nbest=3
       Loading WFSA-format LM...
       Computing best permutation...
       Symbol: 'jibberish' not found in input symbols table.
       Mapping to <unk>
       Time(nsec): 1160492
       28.3986(log10: -12.3333) he goes to the <unk> park
       30.1492(log10: -13.0936) he goes to the park <unk>
       31.4548(log10: -13.6606) he goes to <unk> the park

### Compare
In order to verify the log-probability scores computed by the utility, two scripts are provided.

#### OpenGRM
If OpenGRM is installed, the `grm-perp.py` script may be used. Assuming `text.txt` contains the sentences,

    he goes to the <unk> park
    he goes to the park <unk>
    he goes to <unk> the park

then the following command will compute the probability of individual sentences:

    $ ./grm-perp.py --lm testlm.arpa --test test.txt --arpa
       ngramread --ARPA testlm.arpa > test.fst
       ./get-syms test.fst test.isyms
       INFO: FstImpl::ReadHeader: source: test.fst, fst_type: vector, arc_type: standard, version: 2, flags: 3
       INFO: FstImpl::ReadHeader: source: <unspecfied>, fst_type: vector, arc_type: standard, version: 2, flags: 0
       INFO: FstImpl::ReadHeader: source: <unspecfied>, fst_type: vector, arc_type: standard, version: 2, flags: 0
       INFO: FstImpl::ReadHeader: source: <unspecfied>, fst_type: vector, arc_type: standard, version: 2, flags: 0
       WARNING: OOV probability = 0; OOVs will be ignored in perplexity calculation
       he goes to the <unk> park
                                                ngram  -logprob
         N-gram probability                      found  (base10)
         p( he | <s> )                        = [2gram]  1.46402
         p( goes | he ...)                    = [2gram]  3.06386
         p( to | goes ...)                    = [2gram]  0.771098
         p( the | to ...)                     = [2gram]  1.02492
         p( <unk> | the ...)                  = [2gram]  1.66247
         p( park | <unk> ...)                 = [2gram]  3.49122
         p( </s> | park ...)                  = [2gram]  0.855749
       1 sentences, 6 words, 0 OOVs       
       logprob(base 10)= -12.3333;  perplexity = 57.797
      ...
      ...

#### SRILM
The SRILM `ngram` command may also be used for the same purpose:

    $ ngram -lm testlm.arpa -ppl test.txt -unk -debug 5
      reading 64000 1-grams
      reading 7896695 2-grams
      reading 0 3-grams
      he goes to the <unk> park
         p( he | <s> )     = [2gram] 0.0343541 [ -1.46402 ] / 1
         p( goes | he ...)   = [2gram] 0.000863263 [ -3.06386 ] / 1
         p( to | goes ...)   = [2gram] 0.169396 [ -0.771098 ] / 1
         p( the | to ...)    = [2gram] 0.094423 [ -1.02492 ] / 1
         p( <unk> | the ...) 	 = [2gram] 0.0217534 [ -1.66247 ] / 1
         p( park | <unk> ...)  = [2gram] 0.000322689 [ -3.49122 ] / 1
         p( </s> | park ...) 	 = [2gram] 0.139396 [ -0.855749 ] / 1
      1 sentences, 6 words, 0 OOVs
      0 zeroprobs, logprob= -12.3333 ppl= 57.7969 ppl1= 113.646

#### Additional options
Each of the tools contains a bevy of additional options, which may be used to 
perform modified versions of the above commands.  For example the use of failure
and epsilon transitions may be toggled, and the '<unk>' and sentence-begin and
sentence-end tokens may be set by the user.  See the individual `--help` for 
further details.

Lattice method details
----------------------
Details of the lattice-based method for computing the most likely permutation
are provided in the docs/lattice-method.pdf document.  This includes several
graphical examples of the approach, which is significantly more efficient than
the brute-force STD-based approach.