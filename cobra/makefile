# CC = g++
ifndef CC
  CC = clang++
endif
ifeq ($(CC),cc)
  CC = clang++
endif

CCFLAGS = -g -O3 -isystem tools/ -isystem tools/minisat -isystem src/ -isystem /opt/local/include -Wall -Wextra -pedantic -pthread -std=c++11
LINKWITHSAT = -L tools/picosat -L tools/bliss -L tools/minisat/core -l picosat -l bliss -l minisat
FLEX = flex
BISON = bison -d

# file names
EXEC = cobra-backend
SOURCES = $(filter-out src/main.cpp, $(wildcard src/*.cpp))
OBJECTS = $(SOURCES:.cpp=.o) src/cobra.yy.o

default: cobra
cobra: $(OBJECTS) src/main.o
	@if [ ! -f tools/bliss/libbliss.a ]; then echo "Compiling BLISS..."; $(MAKE) -C tools/bliss; fi
	@if [ ! -f tools/picosat/libpicosat.a ]; then echo "Compiling PICOSAT"; $(MAKE) -C tools/picosat; fi
	@if [ ! -f tools/minisat/core/libminisat.a ]; then echo "Compiling MINISAT"; $(MAKE) libr -C tools/minisat/core; fi
	$(CC) $(OBJECTS) src/main.o $(LINKWITHSAT) -o $(EXEC)

# parser
clean-bison:
	rm src/cobra.tab.cpp src/cobra.tab.hpp src/cobra.yy.c
bison:
	$(BISON) -o src/cobra.tab.cpp src/cobra.ypp
	$(FLEX) -o src/cobra.yy.c src/cobra.l
src/cobra.tab.o: src/cobra.tab.cpp src/cobra.ypp
src/cobra.yy.o: src/cobra.yy.c src/cobra.l
	@# suppress all warnings (compiling c as c++ etc.)
	$(CC) -c -isystem tools/ -std=c++11 -w $< -o $@

# general
%.o: %.cpp %.h
	$(CC) -c $(CCFLAGS) $< -o $@
%.o: %.cpp
	$(CC) -c $(CCFLAGS) $< -o $@

# gtest
GTEST_DIR = ./test
IGNFLAGS += -isystem $(GTEST_DIR)/include
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

$(GTEST_DIR)/gtest-all.o: $(GTEST_SRCS_)
	$(CC) $(IGNFLAGS) -I$(GTEST_DIR) $(CCFLAGS) -c $(GTEST_DIR)/src/gtest-all.cc -o $@

$(GTEST_DIR)/gtest_main.o: $(GTEST_SRCS_)
	$(CC) $(IGNFLAGS) -I$(GTEST_DIR) $(CCFLAGS) -c $(GTEST_DIR)/src/gtest_main.cc -o $@

$(GTEST_DIR)/gtest.a: $(GTEST_DIR)/gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

$(GTEST_DIR)/gtest_main.a: $(GTEST_DIR)/gtest-all.o $(GTEST_DIR)/gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

$(GTEST_DIR)/test.o: $(GTEST_DIR)/test.cpp src/*.h $(GTEST_HEADERS)
	$(CC) $(IGNFLAGS) $(CCFLAGS) -c $(GTEST_DIR)/test.cpp -o $@

$(GTEST_DIR)/test: $(GTEST_DIR)/test.o $(OBJECTS) $(GTEST_DIR)/gtest_main.a
	$(CC) $(IGNFLAGS) $^ $(LINKWITHSAT) -lpthread -o $@

utest: $(GTEST_DIR)/test
	./test/test

utest-clean:
	rm -rf $(GTEST_DIR)/*.o $(GTEST_DIR)/*.a $(GTEST_DIR)/test

# functional tests

ftest: cobra
	@(export COBRA_HOME=`pwd`; cd test/ftest;\
	for t in *.sh; do\
	  printf $$t;\
	  printf "... ";\
	  ./$$t | grep -v 'time' | grep -v 'TIME' | \
             grep -v 'Preprocessing' | grep -v 'Loading' > '.test.out';\
	  diff .test.out `basename $$t .sh`.out;\
	  if [ $$? -eq 0 ]; then echo "OK."; fi\
  done)


.PHONY: utest ftest bison

clean:
	rm -rf src/*.o $(EXEC)

