#include "constraint_handler.hpp"

namespace citcpp {
namespace detail {

void constraint_handler::replace_dont_care_values(
    internal_test_set& test_set) const {

  for (auto& t : test_set.get_list_of_tests()) {
    replace_dont_care_values(t);
  }
}

}  // namespace detail
}  // namespace citcpp
