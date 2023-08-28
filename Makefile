CC = g++
INCLUDES = -I /usr/local/include/ff
PRINTF_FLAG = -DPRINT
CFLAGS = -std=c++17 -pthread
# IF PRINT FLAG IS SET, PRINTF_FLAG IS ADDED TO CFLAGS
ifeq ($(PRINT), true)
	CFLAGS += $(PRINTF_FLAG)
endif
# Comment/Uncomment for not using/using default FF mapping
#CFLAGS += -DNO_DEFAULT_MAPPING
CFLAGS_O3 += -O3 $(CFLAGS)

# Path: src/
BIN = ./
SRCS = ./src/
CPPFILES = $(wildcard $(SRCS)/*.cpp)

.PHONY: all clean
.SUFFIXES:
.SUFFIXES: .cpp .o

TARGET =  $(BIN)FastFlow $(BIN)Threads $(BIN)ThreadPool $(BIN)ThreadPoolM $(BIN)Async $(BIN)Sequential
TARGETNO3 =  $(BIN)NO3FastFlow $(BIN)NO3Threads $(BIN)NO3ThreadPool $(BIN)NO3ThreadPoolM $(BIN)NO3Async $(BIN)NO3Sequential

# PRINT OBJS
OBJS = $(patsubst $(SRCS)/%.c, $(BIN)/%.o, $(BIN))
NO3OBJS = $(patsubst $(SRCS)/%.c, $(BIN)/NO3%.o, $(SRCS))

ThreadPoolM.o: $(SRCS)ThreadPool.cpp
	$(CC) -DMINE $(CFLAGS_O3) -c $< -o $@

NO3ThreadPoolM.o: $(SRCS)ThreadPool.cpp
	$(CC) -DMINE -DNO3 $(CFLAGS)  -c $< -o $@

$(BIN)/%.o: $(SRCS)%.cpp
	$(CC) $(CFLAGS_O3) $(INCLUDES)  -c $< -o $@

$(BIN)/NO3%.o: $(SRCS)%.cpp
	$(CC) -DNO3 $(CFLAGS) $(INCLUDES) -c $< -o $@

all: genFiles $(TARGET) $(TARGETNO3)
	make clean

toCLEAN = $(wildcard *.o) 

clean:
	rm -f $(toCLEAN)

cleanexe:
	rm -f $(TARGET) $(TARGETNO3)

cleanall:
	make clean
	make cleanexe
	rm -rf ./data/

genFiles:
	./src/utils/files.sh

testsbig = 5 6 7 8
testsmall = 1 2 3 4

testsmall: all
	@for test in $(testsmall); do \
		make test$$test; \
		make NO3test$$test; \
	done

testsbig:
	@for test in $(testsbig); do \
		make test$$test; \
	done

tests: testsmall testsbig

testsNames = test1 test2 test3 test4 test5 test6 test7 test8

$(testsNames):
	@for target in $(TARGET); do \
		$$target -i $@.txt -p $@.bin; \
		echo "DONE $@.txt $$target"; \
	done

NO3test1:
	@for target in $(TARGETNO3); do \
		$$target -i test1.txt -p encodec1.bin; \
		echo "DONE test1.txt $$target"; \
	done

NO3test2:
	@for target in $(TARGETNO3); do \
		$$target -i test2.txt -p encodec2.bin; \
		echo "DONE test2.txt $$target"; \
	done

NO3test3:
	@for target in $(TARGETNO3); do \
		$$target -i test3.txt -p encodec3.bin; \
		echo "DONE test3.txt $$target"; \
	done

NO3test4:
	@for target in $(TARGETNO3); do \
		$$target -i test4.txt -p encodec4.bin; \
		echo "DONE test4.txt $$target"; \
	done

HELP:
	@echo "Usage: make [OPTION]... [TARGET]..."
	@echo "OPTION:"
	@echo "  PRINT=true:		Enables printing"
	@echo "TARGET:"
	@echo "  all:			Builds all targets"
	@echo "  clean:		Removes all .o files"
	@echo "  cleanexe:		Removes all executables"
	@echo "  cleanall:		Removes all .o files and executables and data"
	@echo "  tests:		Runs all tests"
	@echo "  testsmall:		Runs tests 1-4"
	@echo "  testsbig:		Runs tests 5-8 -> NEEDS all executables"
	@echo "  testi:		Runs i-th test (i=1..8) -> NEEDS all executables"
	@echo "  NO3testi:		Runs i-th (i=1..4) test with NO3 flag -> NEEDS all NO3 executables"
	@echo "  genFiles:		Generates test files (and all directories)"
	@echo "  HELP:			Shows this message"