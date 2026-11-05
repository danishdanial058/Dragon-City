# Builds and runs the rules tests with the same restrictions Unreal uses (no exceptions, no RTTI).
CXX ?= c++
RULES := ../Source/Wyrmhaven/Rules
CXXFLAGS ?= -std=c++20 -O1 -g -Wall -Wextra -Wshadow -Wconversion -Wno-sign-conversion -Werror -fno-exceptions -fno-rtti
SOURCES := RulesTests.cpp $(wildcard $(RULES)/*.cpp)
BUILD := build

.PHONY: test clean
test: $(BUILD)/rules_tests
	./$(BUILD)/rules_tests

$(BUILD)/rules_tests: $(SOURCES) $(wildcard $(RULES)/*.h)
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(RULES) $(SOURCES) -o $@

clean:
	rm -rf $(BUILD)
