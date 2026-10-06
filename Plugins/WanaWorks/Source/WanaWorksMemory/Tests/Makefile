MODULE := $(abspath ..)
BUILD := /tmp/wana-memory-build
CXX := g++
CC := gcc
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -fno-exceptions -fno-rtti -I$(MODULE)/Portable -I$(MODULE)/ThirdParty/sqlite
CFLAGS := -w -I$(MODULE)/ThirdParty/sqlite

.PHONY: test clean

test:
	mkdir -p $(BUILD)
	rm -f $(BUILD)/*.o
	$(CC) $(CFLAGS) -c $(MODULE)/Private/WanaSqliteAmalgamation.c -o $(BUILD)/sqlite.o
	$(CXX) $(CXXFLAGS) -c $(MODULE)/Tests/StandaloneMain.cpp -o $(BUILD)/main.o
	cd $(BUILD) && $(CXX) $(CXXFLAGS) -c $(MODULE)/Portable/*.cpp
	$(CXX) $(CXXFLAGS) -o $(BUILD)/wana_memory_tests $(BUILD)/*.o -pthread -ldl -lm
	$(BUILD)/wana_memory_tests

clean:
	rm -rf $(BUILD)
