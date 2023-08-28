CC = g++
INCLUDES = -I /usr/local/include/ff
PRINTFLAG = -DPRINT
CFLAGSBASE += -std=c++17 
CFLAGS += -O3 $(CFLAGSBASE)
CFLAGSFF += -DNO_DEFAULT_MAPPING -pthread $(CFLAGS) $(INCLUDES)
CFLAGSFFBASE += -DNO_DEFAULT_MAPPING -pthread $(CFLAGSBASE) $(INCLUDES)

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

FastFlow.o: $(SRCS)FastFlow.cpp
	$(CC) $(CFLAGSFF) -c $< -o $@

NO3FastFlow.o: $(SRCS)FastFlow.cpp
	$(CC) -DNO3 $(CFLAGSFFBASE) -c $< -o $@

ThreadPoolM.o: $(SRCS)ThreadPool.cpp
	$(CC) -DMINE $(CFLAGS) -c $< -o $@

NO3ThreadPoolM.o: $(SRCS)ThreadPool.cpp
	$(CC) -DMINE -DNO3 $(CFLAGSBASE) -c $< -o $@

$(BIN)/%.o: $(SRCS)%.cpp
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN)/NO3%.o: $(SRCS)%.cpp
	$(CC) -DNO3 $(CFLAGSBASE) -c $< -o $@

all:  $(TARGET) $(TARGETNO3)
	make clean

toCLEAN = $(wildcard *.o) 

clean:
	rm -f $(toCLEAN)

cleanall:
	rm -f $(TARGET) $(TARGETNO3)

# Path: data/
DATA = $(wildcard data/testFiles/*.txt)
dataName = $(notdir $(DATA))

