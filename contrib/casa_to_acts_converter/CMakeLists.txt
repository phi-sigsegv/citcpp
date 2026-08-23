cmake_minimum_required(VERSION 3.16)
project(casa_to_acts_converter CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(casa_to_acts_converter casa_to_acts_converter.cpp)
