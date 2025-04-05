PLUGIN = lab1
SRC_DIR = src
CXXFLAGS = -O2 -Wall -std=c++14 -fno-rtti -I$(shell gcc -print-file-name=plugin)/include
LDFLAGS = -shared -fPIC

all: $(PLUGIN).so

$(PLUGIN).so: $(SRC_DIR)/main.cpp
        g++ $(CXXFLAGS) $(LDFLAGS) -o $(PLUGIN).so $(SRC_DIR)/main.cpp

clean:
        rm -f $(PLUGIN).so test   
