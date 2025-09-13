# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)

set(CITCPP_LIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/binom_coeff_table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/citcpp_algo_common.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/citcpp_ipog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/coverage_map.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/for_each_cross_product_elem.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/internal_model.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/ipog_all_value_combinations.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/ipog_horizontal_extension.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/ipog_otf_horizontal_extension.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/ipog_vertical_extension.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/covm_algorithm_uniform_strength.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/citcpp_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/citcpp_covm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_set.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/citcpp_config.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/coverage_measurement.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/citcpp.cpp
)

add_library(citcpp_lib ${CITCPP_LIB_SOURCES})
target_include_directories(citcpp_lib PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>")
set_property(TARGET citcpp_lib PROPERTY OUTPUT_NAME citcpp)
target_link_libraries(citcpp_lib PUBLIC Threads::Threads)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(citcpp_lib PRIVATE -Wno-interference-size)
endif()

