# Get the OS name
OS := $(shell uname)

# Set the compiler and flags based on OS
ifeq ($(OS), Linux)
    CC = g++
    EXEC = ./bin/BTC_Input_Output_Mapper_Linux
    CFLAGS = -Wall -g -c -I/usr/local/include
    LFLAGS = -lcurl
    # TODO: check includes for curl and nlohmann-json
endif

ifeq ($(OS), Darwin)
    # brew install gcc
    CC = g++-14
    EXEC = ./bin/BTC_Input_Output_Mapper_macOS
    CFLAGS = -Wall -g -c -I/opt/homebrew/opt/nlohmann-json/include
    LFLAGS = -lcurl
endif

all: compile link

# Compile source to obj
compile: ./src/main.cpp
	$(CC) $(CFLAGS) ./src/main.cpp -o ./obj/main.o

# TODO: use variables for source and object files

# Link obj to executable
link: ./obj/main.o
	$(CC) -o $(EXEC) ./obj/main.o $(LFLAGS)
