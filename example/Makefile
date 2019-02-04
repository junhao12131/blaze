CXX := mpic++
CXX_WARNING_OPTIONS := -Wall -Wextra -Wno-unused-result
CXXFLAGS := -std=c++14 -O3 -fopenmp $(CXX_WARNING_OPTIONS)
CXXFLAGS := $(CXXFLAGS) -isystem ../../

.PHONY: all clean

.SUFFIXES:

all: wordcount pi

wordcount: wordcount.cc
	$(CXX) $(CXXFLAGS) $^ -o $@

pi: pi.cc
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f wordcount
	rm -f pi
