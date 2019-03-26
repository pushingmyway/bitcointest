cmake_minimum_required(VERSION 3.7)
project(bitcointest)

set(CMAKE_CXX_STANDARD 11)

set(EVENT_DIR /usr/local)

include_directories(${EVENT_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR})

find_library(EVENT event ${EVENT_DIR}/lib NO_DEFAULT_PATH)

file(GLOB SOURCE_FILES "src/*.cpp" rpc/*.cpp univalue/*.cpp)

add_library(mylib ${SOURCE_FILES})

add_executable(eventtest src/init.cpp)

#target_link_libraries(eventtest ${EVENT})
target_link_libraries(eventtest mylib ${EVENT})


add_executable(eventtesttimer src/eventtesttimer.cpp)
target_link_libraries(eventtesttimer ${EVENT})