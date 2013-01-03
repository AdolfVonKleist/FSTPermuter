# Makefile
#
# Copyright (c) [2012-], Josef Robert Novak
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
#  modification, are permitted #provided that the following conditions
#  are met:
#
#  * Redistributions of source code must retain the above copyright 
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above 
#    copyright notice, this list of #conditions and the following 
#    disclaimer in the documentation and/or other materials provided 
#    with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
# OF THE POSSIBILITY OF SUCH DAMAGE.
CC=g++
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
LIBS=-lfst -ldl -lm
endif
ifeq ($(UNAME), Linux)
LIBS=-lfst -ldl -lm -lrt
endif

OUT=arpa-to-wfsa compute-best-permutation get-syms
TMP=*.o
CFLAGS ?= -O2 -Wall
THIRD_PARTIES_INCLUDE ?= -Iutfcpp
EXTRA=$(CFLAGS) $(THIRD_PARTIES_INCLUDE) $(CXXFLAGS) $(CPPFLAGS) 

OBJS=ARPA2WFST.o FstPathFinder.o PermutationLattice.o util.o

all: $(OUT)

%.o: %.cpp
	$(CC) $(EXTRA) -c $(<) -o $(@)

arpa-to-wfsa: util.o ARPA2WFST.o arpa-to-wfsa.cpp
	$(CC) $(EXTRA) $(LDFLAGS) ARPA2WFST.o util.o arpa-to-wfsa.cpp -o arpa-to-wfsa $(LIBS)

compute-best-permutation: FstPathFinder.o PermutationLattice.o util.o compute-best-permutation.cpp
	$(CC) $(EXTRA) $(LDFLAGS) FstPathFinder.o PermutationLattice.o util.o compute-best-permutation.cpp -o compute-best-permutation $(LIBS)

get-syms: get-syms.cpp
	$(CC) $(EXTRA) $(LDFLAGS) get-syms.cpp -o get-syms $(LIBS)

clean:
	$(RM) $(OUT) $(TMP)
