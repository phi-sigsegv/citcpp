cmake_minimum_required(VERSION 3.16)

# set the project name
project(citcpp VERSION 1.0.2)

include(GNUInstallDirs)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(CITCPP_BUILD_TESTS "Enables builds of tests" ON)

set(CITCPP_3RD_PARTY_DIR ${PROJECT_SOURCE_DIR}/3rd_party)

message(STATUS "CITCPP_BUILD_TESTS: " ${CITCPP_BUILD_TESTS})

add_subdirectory(citcpp_lib)

set(CITCPP_EXE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/acts_model_parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_set_parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/parser_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/covm_json.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

configure_file(src/config.hpp.in config.hpp)

add_executable(citcpp_exe ${CITCPP_EXE_SOURCES})
target_include_directories(citcpp_exe PUBLIC "${PROJECT_BINARY_DIR}" "${CITCPP_3RD_PARTY_DIR}/cpp-peglib" "${CITCPP_3RD_PARTY_DIR}/CLI11")
target_link_libraries(citcpp_exe PUBLIC citcpp_lib)
set_property(TARGET citcpp_exe PROPERTY OUTPUT_NAME citcpp)

# /Zc:__cplusplus is required to make __cplusplus accurate
# /Zc:__cplusplus is available starting with Visual Studio 2017 version 15.7
# (according to https://learn.microsoft.com/en-us/cpp/build/reference/zc-cplusplus)
# That version is equivalent to _MSC_VER==1914
# (according to https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=vs-2019)
# CMake's ${MSVC_VERSION} is equivalent to _MSC_VER
# (according to https://cmake.org/cmake/help/latest/variable/MSVC_VERSION.html#variable:MSVC_VERSION)
if ((MSVC) AND (MSVC_VERSION GREATER_EQUAL 1914))
    target_compile_options(citcpp_exe PRIVATE "/Zc:__cplusplus" "/utf-8")
endif()

install(TARGETS citcpp_exe)

# -----------------------------------------------------------------------------
# Unittest
# -----------------------------------------------------------------------------
if(CITCPP_BUILD_TESTS)
  set(CITCPP_MAIN_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
  include(CTest)
  add_subdirectory(tests)
endif(CITCPP_BUILD_TESTS)
