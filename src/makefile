#!/bin/bash

EIDIR=-I/opt/local/include -I/opt/install/include \
	-I/opt/intel/compilers_and_libraries_2017.4.181/mac/mkl/include/ \
	-I.
ELDIR=-L/opt/intel/compilers_and_libraries_2017.4.181/mac/mkl/lib \
	-L/opt/intel/compilers_and_libraries_2017.4.181/mac/tbb/lib/ \
	-L/opt/local/lib

IDIR=../inc
CC=gcc
CFLAGS=-O0 -g3 -std=gnu99 -I$(IDIR) -I../ltsmin/src $(EIDIR)
CXXFLAGS=-O0 -g3 -std=c++11 -I$(IDIR) -I../ltsmin/src $(EIDIR) $(ELDIR)

LIBS=../ltsmin/src/hre/.libs/libhre.a ../ltsmin/src/ltsmin-lib/.libs/libltsmin.a \
	../ltsmin/src/pins-lib/.libs/libpins.a ../ltsmin/src/mc-lib/.libs/libmc.a \
	-lmkl_intel_ilp64 -lmkl_core -lmkl_avx -lmkl_mc -lmkl_intel \
	-lmkl_tbb_thread -DMKL_ILP64 -ltbb -lpopt -lz

ODIR=../bin
_OBJ = Alphabeta.o CSE.o GameTree.o GemPuzzleState.o HexState.o Parser.o \
		PinsState.o PolyState.o main.o prom-pins.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

all: $(ODIR) $(ODIR)/paralleluct

$(ODIR)/paralleluct: $(OBJ)
	$(CXX) -O0 -g3 -o $(ODIR)/$@ $^ $(CXXFLAGS) $(LIBS)
	
$(ODIR):
	mkdir $(ODIR)

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(ODIR)/prom-pins.o:
	$(CC)  -c $(CFLAGS) -o $(ODIR)/prom-pins.o ../ltsmin/src/pins-lib/modules/prom-pins.c

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 


#gcc -O0 -g3  -std=gnu99 -c $(CFLAGS) -o prom-pins.o ltsmin/src/pins-lib/modules/prom-pins.c
#
#for a in `echo "Alphabeta CSE GameTree GemPuzzleState HexState Parser PinsState PolyState main"`; do
#	echo -n $a
#	if [ src/${a}.cpp -nt ${a}.o ]; then
#		g++ -O0 -g3 -std=c++11 -pthread  -c -Iltsmin/src -Iinc/paralleluct/state -Iinc -I/opt/local/include -I/opt/intel/compilers_and_libraries_2017.4.181/mac/mkl/include/ -Isrc -std=c++11 -o ${a}.o src/${a}.cpp
#		echo -e "\tDONE"
#	else
#		echo
#	fi
#done
#
#g++ -O0 -g3 prom-pins.o Alphabeta.o CSE.o GameTree.o GemPuzzleState.o \
#HexState.o Parser.o PinsState.o PolyState.o main.o \
#-L/opt/intel/compilers_and_libraries_2017.4.181/mac/mkl/lib 
#-L/opt/local/lib \
#ltsmin/src/hre/.libs/libhre.a ltsmin/src/ltsmin-lib/.libs/libltsmin.a \
#ltsmin/src/pins-lib/.libs/libpins.a ltsmin/src/mc-lib/.libs/libmc.a \
#-lmkl_intel_ilp64 -lmkl_core -lmkl_avx -lmkl_mc -lmkl_intel -lmkl_tbb_thread \
#-DMKL_ILP64 -m64 -ltbb -lpopt -lz -o paralleluct2
#
