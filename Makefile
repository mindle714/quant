CXXFLAGS=-I/opt/intel/mkl/include -O0 -g 
LDFLAGS=-L/opt/intel/mkl/lib/intel64_lin -lmkl_intel_ilp64 -lmkl_intel_thread -lmkl_core -L/opt/intel/compilers_and_libraries_2019.2.187/linux/compiler/lib/intel64_lin -liomp5 -lpthread -lm -ldl

all: test test2

test: test.o
	g++ --std=c++11 $(CXXFLAGS) test.o $(LDFLAGS) -o test

test.o: test.cpp
	g++ --std=c++11 $(CXXFLAGS) test.cpp $(LDFLAGS) -c

test2: test2.o
	g++ --std=c++11 $(CXXFLAGS) test2.o $(LDFLAGS) -o test2

test2.o: test2.cpp
	g++ --std=c++11 $(CXXFLAGS) test2.cpp $(LDFLAGS) -c

clean:
	rm test test2 test.o test2.o
