enable_testing()

include(${CITCPP_3RD_PARTY_DIR}/doctest/cmake/doctest.cmake)

list(APPEND CITCPP_UNITTESTS 
  test_binom_coeff_table
  test_bitset
  test_cagen
  test_cagen_extend
  test_cagen_relations
  test_cov_measure_result
  test_cov_measure
  test_cov_measure_relations
)

foreach(unittest IN LISTS CITCPP_UNITTESTS)
  add_executable(${unittest} ${unittest}.cpp)
  target_link_libraries(${unittest} citcpp_lib)
  target_include_directories(${unittest} PRIVATE "${CITCPP_3RD_PARTY_DIR}/doctest")
  doctest_discover_tests(${unittest})
endforeach()

