TARGET = compromutator

.PHONY : all clean

SRC_PATH = ./src
HEADER_SPACE_PATH = $(SRC_PATH)/header_space
DETECTOR_PATH = $(SRC_PATH)/detector
DEPENDENCY_GRAPH_PATH = $(SRC_PATH)/dependency_graph

HEADER_SPACE_SRC += $(HEADER_SPACE_PATH)/HeaderSpace.cpp

DETECTOR_SRC += $(DETECTOR_PATH)/Detector.cpp
DETECTOR_SRC += $(DETECTOR_PATH)/FlowPredictor.cpp

DEPENDENCY_GRAPH_SRC += $(DEPENDENCY_GRAPH_PATH)/DependencyGraph.cpp
DEPENDENCY_GRAPH_SRC += $(DEPENDENCY_GRAPH_PATH)/Network.cpp
DEPENDENCY_GRAPH_SRC += $(DEPENDENCY_GRAPH_PATH)/Topology.cpp

all: ./src/Main.cpp $(HEADER_SPACE_SRC) $(DETECTOR_SRC) $(DEPENDENCY_GRAPH_SRC) headerspace.o
	g++ -std=c++11 -g -o $(TARGET) $^
	#rm -rf *.o

headerspace.o: array.o hs.o
	ld -o headerspace.o -r $^ 

array.o: ./src/header_space/array.c
	gcc -g -std=gnu99 -o array.o -c $^

hs.o: ./src/header_space/hs.c
	gcc -g -std=gnu99 -o hs.o -c $^

clean:
	rm -rf $(TARGET) *.o