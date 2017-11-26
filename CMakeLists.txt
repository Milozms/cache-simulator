cmake_minimum_required(VERSION 3.8)
project(cache)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES storage.h main.cc cache.cc cache.h memory.h memory.cc def.h)
add_executable(cache ${SOURCE_FILES})