#set(FLUID_BASE_FILES ${CMAKE_BINARY_DIR}/libfluid_base)
#if (NOT EXISTS ${FLUID_BASE_FILES}/config.status)
#    message("Configuring libfluid_base...")
#
#    file(MAKE_DIRECTORY ${FLUID_BASE_FILES})
#    execute_process(
#        COMMAND ${CMAKE_SOURCE_DIR}/third_party/libfluid_base/configure
#            --prefix=${CMAKE_BINARY_DIR}/fluid --disable-tls
#            --enable-static=no
#        WORKING_DIRECTORY ${FLUID_BASE_FILES}
#        RESULT_VARIABLE FLUID_BASE_CONFIGURED
#    )
#
#    if (NOT ${FLUID_BASE_CONFIGURED} EQUAL 0)
#        message( FATAL_ERROR "Can't configure libfluid_base" )
#    endif()
#endif()
#
#set(FLUID_MSG_FILES ${CMAKE_BINARY_DIR}/libfluid_msg)
#if (NOT EXISTS ${FLUID_MSG_FILES}/config.status)
#    message("Configuring libfuild_msg...")
#
#    file(MAKE_DIRECTORY ${FLUID_MSG_FILES})
#    execute_process(
#        COMMAND ${CMAKE_SOURCE_DIR}/third_party/libfluid_msg/configure
#            --prefix=${CMAKE_BINARY_DIR}/fluid
#            CXXFLAGS=-DIFHWADDRLEN=6 # OS X compilation fix
#        WORKING_DIRECTORY ${FLUID_MSG_FILES}
#        RESULT_VARIABLE FLUID_MSG_CONFIGURED
#    )
#
#    if (NOT ${FLUID_MSG_CONFIGURED} EQUAL 0)
#        message( FATAL_ERROR "Can't configure libfluid_msg" )
#    endif()
#endif()
#
## Build libfluid
#add_custom_target(fluid_msg
#    COMMAND make install
#    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/libfluid_msg
#    COMMENT "Building libfluid_msg"
#)
#add_custom_target(fluid_base
#    COMMAND make install
#    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/libfluid_base
#    COMMENT "Building libfluid_base"
#)
#
#add_custom_target(libfluid)
#add_dependencies(libfluid
#    fluid_msg
#    fluid_base
#)

# Build fluid_base
set(FLUID_BASE_SRC_DIR ${CMAKE_SOURCE_DIR}/third_party/libfluid_base/fluid)
set(FLUID_BASE_SRC
    ${FLUID_BASE_SRC_DIR}/base/EventLoop.hh
    ${FLUID_BASE_SRC_DIR}/base/EventLoop.cc
    ${FLUID_BASE_SRC_DIR}/base/BaseOFConnection.hh
    ${FLUID_BASE_SRC_DIR}/base/BaseOFConnection.cc
    ${FLUID_BASE_SRC_DIR}/base/BaseOFServer.hh
    ${FLUID_BASE_SRC_DIR}/base/BaseOFServer.cc
    ${FLUID_BASE_SRC_DIR}/base/BaseOFClient.hh
    ${FLUID_BASE_SRC_DIR}/base/BaseOFClient.cc
    ${FLUID_BASE_SRC_DIR}/OFConnection.hh
    ${FLUID_BASE_SRC_DIR}/OFConnection.cc
    ${FLUID_BASE_SRC_DIR}/OFServerSettings.hh
    ${FLUID_BASE_SRC_DIR}/OFServerSettings.cc
    ${FLUID_BASE_SRC_DIR}/OFServer.hh
    ${FLUID_BASE_SRC_DIR}/OFServer.cc
    ${FLUID_BASE_SRC_DIR}/base/of.hh
    ${FLUID_BASE_SRC_DIR}/OFClient.hh
    ${FLUID_BASE_SRC_DIR}/OFClient.cc
    ${FLUID_BASE_SRC_DIR}/TLS.hh
    ${FLUID_BASE_SRC_DIR}/TLS.cc
)

# Build fluid_msg
set(FLUID_MSG_SRC_DIR ${CMAKE_SOURCE_DIR}/third_party/libfluid_msg/fluid)
set(FLUID_MSG_UTIL_SRC
    ${FLUID_MSG_SRC_DIR}/util/ethaddr.hh
    ${FLUID_MSG_SRC_DIR}/util/ethaddr.cc
    ${FLUID_MSG_SRC_DIR}/util/ipaddr.hh
    ${FLUID_MSG_SRC_DIR}/util/ipaddr.cc
    ${FLUID_MSG_SRC_DIR}/util/util.h
)
set(FLUID_MSG_OFCOMMON_SRC
    ${FLUID_MSG_SRC_DIR}/ofcommon/msg.hh
    ${FLUID_MSG_SRC_DIR}/ofcommon/msg.cc
    ${FLUID_MSG_SRC_DIR}/ofcommon/action.hh
    ${FLUID_MSG_SRC_DIR}/ofcommon/action.cc
    ${FLUID_MSG_SRC_DIR}/ofcommon/common.hh
    ${FLUID_MSG_SRC_DIR}/ofcommon/common.cc
    ${FLUID_MSG_SRC_DIR}/ofcommon/openflow-common.hh
)
set(FLUID_MSG_OF10_SRC
    ${FLUID_MSG_SRC_DIR}/of10/of10action.hh
    ${FLUID_MSG_SRC_DIR}/of10/of10action.cc
    ${FLUID_MSG_SRC_DIR}/of10/of10common.hh
    ${FLUID_MSG_SRC_DIR}/of10/of10common.cc
    ${FLUID_MSG_SRC_DIR}/of10/of10match.hh
    ${FLUID_MSG_SRC_DIR}/of10/of10match.cc
    ${FLUID_MSG_SRC_DIR}/of10/openflow-10.h
)
set(FLUID_MSG_OF13_SRC
    ${FLUID_MSG_SRC_DIR}/of13/of13instruction.hh
    ${FLUID_MSG_SRC_DIR}/of13/of13instruction.cc
    ${FLUID_MSG_SRC_DIR}/of13/of13action.hh
    ${FLUID_MSG_SRC_DIR}/of13/of13action.cc
    ${FLUID_MSG_SRC_DIR}/of13/of13match.hh
    ${FLUID_MSG_SRC_DIR}/of13/of13match.cc
    ${FLUID_MSG_SRC_DIR}/of13/of13meter.hh
    ${FLUID_MSG_SRC_DIR}/of13/of13meter.cc
    ${FLUID_MSG_SRC_DIR}/of13/of13common.hh
    ${FLUID_MSG_SRC_DIR}/of13/of13common.cc
    ${FLUID_MSG_SRC_DIR}/of13/openflow-13.h
)
set(FLUID_MSG_SRC
    ${FLUID_MSG_UTIL_SRC}
    ${FLUID_MSG_OFCOMMON_SRC}
    ${FLUID_MSG_OF10_SRC}
    ${FLUID_MSG_OF13_SRC}
    ${FLUID_MSG_SRC_DIR}/of10msg.hh
    ${FLUID_MSG_SRC_DIR}/of10msg.cc
    ${FLUID_MSG_SRC_DIR}/of13msg.hh
    ${FLUID_MSG_SRC_DIR}/of13msg.cc
)

include_directories(SYSTEM
    ${CMAKE_SOURCE_DIR}/third_party/libfluid_base
    ${CMAKE_SOURCE_DIR}/third_party/libfluid_base/fluid
    ${CMAKE_SOURCE_DIR}/third_party/libfluid_msg
    ${CMAKE_SOURCE_DIR}/third_party/libfluid_msg/fluid
)

find_package(Threads MODULE REQUIRED)
find_package(OpenSSL MODULE REQUIRED)

if (EVENT_INCLUDE_DIR AND EVENT_LIBRARY)
    # Already in cache, be silent
    set(EVENT_FIND_QUIETLY TRUE)
endif (EVENT_INCLUDE_DIR AND EVENT_LIBRARY)

find_path(EVENT_INCLUDE_DIR event.h
    PATHS /usr/include
    PATH_SUFFIXES event
    )

find_library(EVENT_LIBRARY
    NAMES event
    PATHS /usr/lib /usr/local/lib
)
set(EVENT_LIBRARIES ${EVENT_LIBRARY})

find_library(EVENT_CORE_LIBRARY
    NAMES event_core
    PATHS /usr/lib /usr/local/lib
    )
set(EVENT_CORE_LIBRARIES ${EVENT_CORE_LIBRARY})

find_library(EVENT_OPENSSL_LIBRARY
    NAMES event_openssl
    PATHS /usr/lib /usr/local/lib
)
set(EVENT_OPENSSL_LIBRARIES ${EVENT_OPENSSL_LIBRARY})

find_library(EVENT_PTHREADS_LIBRARY
    NAMES event_pthreads
    PATHS /usr/lib /usr/local/lib
    )
set(EVENT_PTHREADS_LIBRARIES ${EVENT_PTHREADS_LIBRARY})

add_library(fluid_base STATIC ${FLUID_BASE_SRC})
add_library(fluid_msg STATIC ${FLUID_MSG_SRC})

set_target_properties(fluid_base fluid_msg PROPERTIES
    COMPILE_FLAGS "-pthread -w"
    CXX_STANDARD 98
)

target_link_libraries(fluid_base
    ${CMAKE_THREAD_LIBS_INIT}
    ${EVENT_LIBRARIES}
    ${EVENT_CORE_LIBRARY}
    ${EVENT_OPENSSL_LIBRARIES}
    ${EVENT_PTHREADS_LIBRARY}
    ${OPENSSL_LIBRARIES}
)
