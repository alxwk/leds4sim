cmake_minimum_required(VERSION 3.10)

project(leds4sim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-Wall -Ofast")

include_directories(
    src
    src/moza_protocol
)

add_executable(leds4sim
    src/main.cpp
    src/indicator.cpp
    src/indicator.h
    src/moza_protocol/rgb.cpp
    src/moza_protocol/proto.cpp
    src/moza_protocol/get_reply.cpp
    src/moza_protocol/proto.h
    src/moza_protocol/rgb.h
)

target_link_libraries(leds4sim
    serial
    config++
    xdg-basedir
)
