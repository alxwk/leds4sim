cmake_minimum_required(VERSION 3.10)

project(leds4sim)

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_FLAGS "-fPIC -Wall")
set(CMAKE_CXX_FLAGS "-Wall")
#set(CMAKE_SHARED_LINKER_FLAGS, "-Wl,-soname,scs_sdk_plugin.so")

#include_directories(scs-telemetry/inc)
#include_directories(scs_sdk/include)
include_directories(
src
src/moza_protocol
)

add_executable(leds4sim
    src/main.cpp
    src/threshold.cpp
    src/moza_protocol/rgb.cpp
    src/moza_protocol/proto.cpp
    src/moza_protocol/get_reply.cpp
    src/moza_protocol/proto.h
    src/moza_protocol/rgb.h
)

target_link_libraries(leds4sim
    serial
    config++
)
