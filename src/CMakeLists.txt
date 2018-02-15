
add_library(header_space STATIC
        header_space/array.c
        header_space/array.h
        header_space/hs.c
        header_space/hs.h
        header_space/list.h
        header_space/map.h
        header_space/util.h)

add_library(detector STATIC
        network/Graph.hpp
        network/DependencyGraph.cpp
        network/DependencyGraph.hpp
        flow_predictor/FlowPredictor.cpp
        flow_predictor/FlowPredictor.hpp
        flow_predictor/Node.hpp
        flow_predictor/PathScan.hpp
        flow_predictor/PathScan.cpp
        flow_predictor/Stats.hpp
        flow_predictor/Stats.cpp
        flow_predictor/Timestamp.hpp
        header_space/HeaderSpace.cpp
        header_space/HeaderSpace.hpp
        openflow/Types.hpp
        network/Switch.hpp
        network/Switch.cpp
        network/Network.cpp
        network/Network.hpp
        network/Rule.cpp
        network/Rule.hpp
        network/Types.hpp
        network/Vertex.hpp
        openflow/Action.hpp
        openflow/Types.hpp
        openflow/Parser.cpp
        openflow/Parser.hpp
        openflow/Mapping.hpp
        openflow/Mapping.cpp
        proxy/Proxy.hpp
        proxy/ConnectionManager.hpp
        proxy/ConnectionManager.cpp
        proxy/Event.hpp
        Controller.hpp
        Controller.cpp
        Detector.cpp
        Detector.hpp
        NetworkSpace.cpp
        NetworkSpace.hpp
        ConcurrencyPrimitives.hpp
        Compromutator.cpp
        Compromutator.hpp)

target_link_libraries(detector
    header_space
    fluid_msg
    fluid_base
)