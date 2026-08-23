cmake_minimum_required(VERSION 3.16)
project(dimecs_to_acts_converter CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(dimacs_to_acts_converter dimacs_to_acts_converter.cpp)
