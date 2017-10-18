
#include <typeinfo>

#include <iostream>
#include <list>
#include <map>
#include <string.h>
#include <vector>

#include "Common.hpp"
#include "NetworkSpace.hpp"
#include "./detector/Detector.hpp"
#include "Test.hpp"

using namespace std;

int main(int argc, char* argv[])
{
    DependencyGraphTest graph_test;

    // Run tests
    graph_test.simpleDependencyTest();
}
