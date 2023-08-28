CC = g++
INCLUDES = -I /usr/local/include/ff
PRINTF_FLAG = -DPRINT
CFLAGS = -std=c++17 -pthread
# IF PRINT FLAG IS SET, PRINTF_FLAG IS ADDED TO CFLAGS
ifeq ($(PRINT), true)
	CFLAGS += $(PRINTF_FLAG)
endif
# Comment/Uncomment for not using/using default FF mapping
CFLAGS += -DNO_DEFAULT_MAPPING
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

all:  $(TARGET) $(TARGETNO3)
	make clean

toCLEAN = $(wildcard *.o) 

clean:
	rm -f $(toCLEAN)

cleanall:
	rm -f $(TARGET) $(TARGETNO3)

testsbig = 5 6 7 8
testsmall = 1 2 3 4

testsmall:
	@for test in $(testsmall); do \
		make test$$test; \
	done

testsbig:
	@for test in $(testsbig); do \
		make test$$test; \
	done

tests: testsmall testsbig

testsNames = test1 test2 test3 test4 test5 test6 test7 test8

$(testsNames): $(TARGET) $(TARGETNO3)
	@for target in $(TARGET); do \
		$$target -i $@.txt -p $@.bin; \
		echo "DONE $@.txt $$target"; \
	done
	@for target in $(TARGETNO3); do \
		$$target -i $@.txt -p $@.bin; \
		echo "DONE $@.txt $$target"; \
	done
	make clean
