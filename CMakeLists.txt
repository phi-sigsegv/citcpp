cmake_minimum_required(VERSION 3.16)

# set the project name
project(citcpp VERSION 3.0.0)

include(GNUInstallDirs)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(CITCPP_BUILD_TESTS "Enables builds of tests" ON)
option(ENABLE_ASAN "Enable AddressSanitizer (ASan)" OFF)
option(ENABLE_CPPCHECK "Enable analysis using cppcheck" OFF)
option(ENABLE_CLANG_TIDY "Enable analysis using clang-tidy" OFF)

set(CITCPP_3RD_PARTY_DIR ${PROJECT_SOURCE_DIR}/3rd_party)

message(STATUS "CITCPP_BUILD_TESTS: " ${CITCPP_BUILD_TESTS})
message(STATUS "ENABLE_ASAN: " ${ENABLE_ASAN})
message(STATUS "ENABLE_CPPCHECK: " ${ENABLE_CPPCHECK})
message(STATUS "ENABLE_CLANG_TIDY: " ${ENABLE_CLANG_TIDY})

set(LACE_NATIVE_OPT OFF)
set(SYLVAN_NATIVE_OPT OFF)
set(SYLVAN_BUILD_CPP OFF)

if(BUILD_SHARED_LIBS)
  set(LACE_ENABLE_PIC ON)
  set(SYLVAN_ENABLE_PIC ON)
else()
  set(LACE_ENABLE_PIC OFF)
  set(SYLVAN_ENABLE_PIC OFF)
endif()

# Generates data necessary for clang-tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(ENABLE_ASAN)
  if(MSVC)
    add_compile_options(/fsanitize=address)
  else()
    # GCC and Clang
    # -fno-omit-frame-pointer results in clean, complete callstacks in error reports
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
  endif()
endif()

# We want lace and sylvan to be static libraries even if BUILD_SHARED_LIBS is ON.
# Sylvan is already static by default, but Lace respects BUILD_SHARED_LIBS.
set(OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(${CITCPP_3RD_PARTY_DIR}/lace EXCLUDE_FROM_ALL)
add_subdirectory(${CITCPP_3RD_PARTY_DIR}/sylvan EXCLUDE_FROM_ALL)
set(BUILD_SHARED_LIBS ${OLD_BUILD_SHARED_LIBS})

add_subdirectory(citcpp_lib)

set(CITCPP_EXE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/covm_json.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

configure_file(src/config.hpp.in config.hpp)

add_executable(citcpp_exe ${CITCPP_EXE_SOURCES})
target_include_directories(citcpp_exe PUBLIC "${PROJECT_BINARY_DIR}" "${CITCPP_3RD_PARTY_DIR}/CLI11")
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
  set(BUILD_TESTING ON)
  include(CTest)
  add_subdirectory(tests)
endif(CITCPP_BUILD_TESTS)
