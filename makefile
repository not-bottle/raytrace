CXX = g++

FILE = raytrace
COMPUTE = compute
OTHERS = src/glad.c 
LINK = src/libassimp.so src/libassimp.so.6 src/libassimp.so.6.0.2

LDFLAGS = -lSDL2 -lGL -lX11 -lpthread -lXrandr -lXi -ldl

CPLUS_INCLUDE_PATH = ./include

all: $(FILE).cpp
	$(CXX) $(FILE).cpp $(OTHERS) -I$(CPLUS_INCLUDE_PATH) -L $(LINK) $(LDFLAGS) -o $(FILE)

compute: $(COMPUTE).cpp
	$(CXX) $(COMPUTE).cpp $(OTHERS) -I$(CPLUS_INCLUDE_PATH) -L $(LINK) $(LDFLAGS) -o $(COMPUTE)
