CXX = g++

FILE = raytrace
COMPUTE = compute
OTHERS = src/glad.c

LDFLAGS = -lSDL2 -lGL -lX11 -lpthread -lXrandr -lXi -ldl

CPLUS_INCLUDE_PATH = ./include

all: $(FILE).cpp
	$(CXX) $(FILE).cpp $(OTHERS) -I$(CPLUS_INCLUDE_PATH) $(LDFLAGS) -o $(FILE)

compute: $(COMPUTE).cpp
	$(CXX) $(COMPUTE).cpp $(OTHERS) -I$(CPLUS_INCLUDE_PATH) $(LDFLAGS) -o $(COMPUTE)
