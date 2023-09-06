CC = g++ -pthread
THREADS = 1
INCLUDES = -I /usr/local/include/ff
PRINTF_FLAG = -DPRINT
THREAD_FLAG = -t $(THREADS)
CFLAGS = -std=c++17
# IF PRINT FLAG IS SET, PRINTF_FLAG IS ADDED TO CFLAGS
ifeq ($(PRINT), true)
	CFLAGS += $(PRINTF_FLAG)
endif
ifeq ($(FTH), true)
	CFLAGS += -DFTH
endif
ifeq ($(TIME), true)
	CFLAGS += -DTIME
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

TARGET =  $(BIN)FastFlow $(BIN)ThreadPool $(BIN)Sequential
TARGETNO3 =  $(BIN)NO3FastFlow $(BIN)NO3ThreadPool $(BIN)NO3Sequential

# PRINT OBJS
OBJS = $(patsubst $(SRCS)/%.c, $(BIN)/%.o, $(BIN))
NO3OBJS = $(patsubst $(SRCS)/%.c, $(BIN)/NO3%.o, $(SRCS))

$(BIN)ThreadPoolM.o: $(SRCS)ThreadPool.cpp
	$(CC) $(CFLAGS_O3) -c $< -o $@

$(BIN)FastFlow.o: $(SRCS)FastFlow.cpp
	$(CC) -pthread $(CFLAGS_O3) $(INCLUDES) -c $< -o $@

$(BIN)NO3FastFlow.o: $(SRCS)FastFlow.cpp
	$(CC) -pthread $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BIN)/%.o: $(SRCS)%.cpp
	$(CC) $(CFLAGS_O3) -c $< -o $@

$(BIN)/NO3%.o: $(SRCS)%.cpp
	$(CC) -DNO3 $(CFLAGS) -c $< -o $@

all: genFiles $(TARGET)
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

testsverybig = 7 8
testsbig = 5 6
testsmall = 2 3 4

testsmall:
	@for test in $(testsmall); do \
		make test$$test; \
	done

testsbig:
	@for test in $(testsbig); do \
		make test$$test; \
	done

testsverybig:
	@for test in $(testsverybig); do \
		make test$$test; \
	done

testtiny:
	make test1

tests: testsmall testsbig testsverybig
testscores: testsmall testsbig


testsNames = test1 test2 test3 test4 test5 test6 test7 test8

SEQ=$(BIN)Sequential -i $@.txt -p enc$@.bin
TP=$(BIN)ThreadPool -i $@.txt -p enc$@.bin
FF=$(BIN)FastFlow -i $@.txt -p enc$@.bin
# IF FLAG IS GREATER THAN 1, THREAD_FLAG IS ADDED TO CFLAGS
ifneq ($(THREADS), 0)
	TP += $(THREAD_FLAG)
	FF += $(THREAD_FLAG)
endif


$(testsNames):
	@$(SEQ)
	@echo "Done $@.txt Sequential"
	@$(FF)
	@echo "Done $@.txt FastFlow"
	@$(TP)
	@echo "Done $@.txt ThreadPool"
	
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
