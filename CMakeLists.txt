cmake_minimum_required(VERSION 3.16)

# set the project name
project(citcpp VERSION 1.0.0)

include(GNUInstallDirs)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

add_subdirectory(citcpp_lib)

set(CITCPP_EXE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

configure_file(src/config.hpp.in config.hpp)

add_executable(citcpp_exe ${CITCPP_EXE_SOURCES})
target_include_directories(citcpp_exe PUBLIC "${PROJECT_BINARY_DIR}")
target_link_libraries(citcpp_exe PUBLIC citcpp_lib)

set_property(TARGET citcpp_exe PROPERTY OUTPUT_NAME citcpp)

