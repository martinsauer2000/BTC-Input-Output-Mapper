# Get the OS name
OS := $(shell uname)

# Set the compiler and flags based on OS
ifeq ($(OS), Linux)
    CC = g++
    EXEC = ./bin/my_program_linux
endif

ifeq ($(OS), Darwin)
    CC = g++-14
    EXEC = ./bin/my_program_mac
endif

# Compiler Flags
CFLAGS = -Wall -g -c

all: compile link

# Compile source to obj
compile: ./src/main.cpp
	$(CC) $(CFLAGS) ./src/main.cpp -o ./obj/main.o

# Link obj to executable
link: ./obj/main.o
	$(CC) -o $(EXEC) ./obj/main.o