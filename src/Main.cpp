
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
    Test simple_test;

    // Run tests
    //simple_test.simpleDependencyTest();
    std::cout<<"latestDiffTest = "
             <<simple_test.latestDiffTest()<<std::endl;
    std::cout<<"simpleFlowPredictorTest = "
             <<simple_test.simpleFlowPredictorTest()<<std::endl;
}
