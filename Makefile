PROJECT_NAME = dantto4k

SRC_DIR = src
OBJ_DIR = build

SRC_FILES = $(filter-out $(SRC_DIR)/bonTuner.cpp $(SRC_DIR)/dllmain.cpp, $(wildcard $(SRC_DIR)/*.cpp))

OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

OPENSSL_INC = $(shell pkg-config --cflags-only-I openssl)
OPENSSL_LIB = $(shell pkg-config --libs openssl)

TSDUCK_INC = $(shell pkg-config --cflags-only-I tsduck)
TSDUCK_LIB = $(shell pkg-config --libs tsduck)

PCSC_INC = $(shell pkg-config --cflags-only-I libpcsclite)
PCSC_LIB = $(shell pkg-config --libs libpcsclite)

CXX = g++
CXXFLAGS = -std=c++20 -Wall -maes -msse4.1 $(OPENSSL_INC) $(TSDUCK_INC) $(PCSC_INC) $(YAML_CPP_INC) -Ithirdparty/asio/asio/include
LDFLAGS = $(OPENSSL_LIB) $(TSDUCK_LIB) $(PCSC_LIB) $(YAML_CPP_LIB)

EXEC = $(OBJ_DIR)/$(PROJECT_NAME)

all: $(EXEC)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(EXEC): $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) $(LDFLAGS) -o $(EXEC)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

install:
	cp $(EXEC) /usr/local/bin/$(PROJECT_NAME)

.PHONY: all clean install