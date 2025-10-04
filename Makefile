CXX = g++
TARGET = drugsim
SRC = $(wildcard src/*.cpp)
HEADERS = $(wildcard include/*.hpp)
PCH_HEADER = include/pch.hpp
PCH = $(PCH_HEADER).gch
FLAGS = -Iinclude -std=c++20 -Wall

$(TARGET): $(SRC) $(HEADERS) $(PCH)
	$(CXX) $(FLAGS) -o $(TARGET) $(SRC)

$(PCH): $(PCH_HEADER)
	$(CXX) $(FLAGS) -x c++-header $(PCH_HEADER) -o $(PCH)
