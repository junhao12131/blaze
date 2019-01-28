# Default options.
CXX := mpic++
CXX_WARNING_OPTIONS := -Wall -Wextra -Wno-unused-result
CXXFLAGS := -std=c++14 -O3 -fopenmp $(CXX_WARNING_OPTIONS)
LDLIBS := -pthread -lpthread
SRC_DIR := src
TEST_DIR := test
BUILD_DIR := build
VENDOR_DIR := vendor
TEST_EXE := blaze_test.out

# Load Makefile.config if exists.
LOCAL_MAKEFILE := local.mk
ifneq ($(wildcard $(LOCAL_MAKEFILE)),)
	include $(LOCAL_MAKEFILE)
endif

# Link Google Perf if available.
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
	TOOLS_DIR := $(HOME)/tools
	GPERFTOOLS_DIR := $(TOOLS_DIR)/gperftools
	ifneq ($(wildcard $(GPERFTOOLS_DIR)),)
		LDLIBS := -L $(GPERFTOOLS_DIR)/lib $(LDLIBS) -ltcmalloc
	endif
endif

# Sources and intermediate objects.
CXXFLAGS := $(CXXFLAGS) -I $(VENDOR_DIR)
TEST := $(shell find $(TEST_DIR) -name "*.cc")
HEADERS := $(shell find $(SRC_DIR) -name "*.h" -or -name "*.inl")
TEST_OBJS := $(TEST:$(TEST_DIR)/%.cc=$(BUILD_DIR)/%.o)
GTEST_DIR := $(VENDOR_DIR)/googletest/googletest
GMOCK_DIR := $(VENDOR_DIR)/googletest/googlemock
GTEST_ALL_SRC := ${GTEST_DIR}/src/gtest-all.cc
GMOCK_ALL_SRC := ${GMOCK_DIR}/src/gmock-all.cc
TEST_MAIN_SRC := gtest_main_mpi.cc
TEST_MAIN_OBJ := $(BUILD_DIR)/gtest_main.o
TEST_CXXFLAGS := $(CXXFLAGS) -isystem $(GTEST_DIR)/include -isystem $(GMOCK_DIR)/include -pthread
TEST_LIB := $(BUILD_DIR)/libgtest.a

.PHONY: all test test_mpi clean

.SUFFIXES:

all: test

test: $(TEST_EXE)
	./$(TEST_EXE) --gtest_filter=-BenchmarkTest.*

test_mpi: $(TEST_EXE)
	OMP_NUM_THREADS=2 mpirun -n 4 ./$(TEST_EXE) --gtest_filter=-*BenchmarkTest.*

clean:
	rm -rf $(BUILD_DIR)
	rm -f ./$(TEST_EXE)

$(TEST_EXE): $(TEST_OBJS) $(OBJS) $(TEST_MAIN_SRC) $(TEST_LIB) 
	$(CXX) $(TEST_CXXFLAGS) $(TEST_OBJS) $(OBJS) $(TEST_MAIN_SRC) $(TEST_LIB) -o $(TEST_EXE) $(LDLIBS)

$(BUILD_DIR)/gtest-all.o: $(GTEST_ALL_SRC)
	mkdir -p $(@D) && $(CXX) $(TEST_CXXFLAGS) -I$(GTEST_DIR) -I$(GMOCK_DIR) -c $(GTEST_ALL_SRC) -o $@

$(BUILD_DIR)/gmock-all.o: $(GMOCK_ALL_SRC)
	mkdir -p $(@D) && $(CXX) $(TEST_CXXFLAGS) -I$(GTEST_DIR) -I$(GMOCK_DIR) -c $(GMOCK_ALL_SRC) -o $@

$(TEST_LIB): $(BUILD_DIR)/gtest-all.o $(BUILD_DIR)/gmock-all.o
	$(AR) $(ARFLAGS) $@ $(BUILD_DIR)/gtest-all.o $(BUILD_DIR)/gmock-all.o

$(TEST_OBJS): $(BUILD_DIR)/%.o: $(TEST_DIR)/%.cc $(HEADERS)
	mkdir -p $(@D) && $(CXX) $(TEST_CXXFLAGS) -c $< -o $@
