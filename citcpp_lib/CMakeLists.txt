# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)

set(CITCPP_LIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/binom_coeff_table.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/citcpp_ipog_base.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/citcpp_ipog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/functor_executor_lace.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/internal_model.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/ipog_all_value_combinations.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/citcpp_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/citcpp_covm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/constraint_evaluator.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/constraint_handler.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/constraint_handler_void.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/constraint_handler_sylvan_ldd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/model_simplifier.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/citcpp_sylvan_ldd.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/idd_variable_ordering.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/detail/lace_lifecycle.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/parameter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/model.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_set.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/citcpp_config.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/coverage_measurement.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/citcpp.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/constraints.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/parser_utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/acts_model_parser.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/test_set_parser.cpp
)

add_library(citcpp_lib ${CITCPP_LIB_SOURCES})

target_include_directories(citcpp_lib PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
  PRIVATE "${CITCPP_3RD_PARTY_DIR}/cpp-peglib")
set_property(TARGET citcpp_lib PROPERTY OUTPUT_NAME citcpp)
target_link_libraries(citcpp_lib PUBLIC Threads::Threads PRIVATE sylvan::sylvan)

if(NOT MSVC)
  target_compile_options(citcpp_lib PRIVATE
    -Wall
    -Wextra
    -Wpedantic          # Strict ISO C++ compliance
    -Wshadow           # Variable shadowing
    -Wnon-virtual-dtor # Missing virtual destructors
    -Wnull-dereference # Null pointer dereferences
    -Wconversion       # Implicit type conversions losing precision
    -Wold-style-cast   # Use C++ static_cast instead of C-style casts
    -Wno-interference-size
  )
endif()

install(TARGETS citcpp_lib
  EXPORT citcpp-targets
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/citcpp
  TYPE INCLUDE
)

install(EXPORT citcpp-targets
  FILE citcpp-targets.cmake
  NAMESPACE citcpp::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)

include(CMakePackageConfigHelpers)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/citcpp-config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/cmake/citcpp-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/citcpp-config-version.cmake
  VERSION ${citcpp_VERSION}
  COMPATIBILITY SameMinorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/citcpp-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/citcpp-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)
