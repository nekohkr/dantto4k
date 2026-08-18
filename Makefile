PROJECT_NAME = dantto4k

SRC_DIR = src
OBJ_DIR = build

SRC_FILES = $(filter-out $(SRC_DIR)/bonTuner.cpp $(SRC_DIR)/dllmain.cpp, \
	$(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/ttml/*.cpp))

OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

TSDUCK_INC = $(shell pkg-config --cflags-only-I tsduck)
TSDUCK_LIB = $(shell pkg-config --libs tsduck)

PCSC_INC = $(shell pkg-config --cflags-only-I libpcsclite)
PCSC_LIB = $(shell pkg-config --libs libpcsclite)

CXX = g++
CXXFLAGS = -std=c++20 -Wall -maes -msse4.1 $(TSDUCK_INC) $(PCSC_INC) -I$(SRC_DIR) -Ithirdparty/asio/asio/include
LDFLAGS = $(TSDUCK_LIB) $(PCSC_LIB)

EXEC = $(OBJ_DIR)/$(PROJECT_NAME)

all: $(EXEC)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(EXEC): $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) $(LDFLAGS) -o $(EXEC)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

install:
	cp $(EXEC) /usr/local/bin/$(PROJECT_NAME)

.PHONY: all clean install
