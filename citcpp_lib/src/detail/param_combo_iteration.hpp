#ifndef DETAIL_PARAM_COMBO_ITERATION_HPP_
#define DETAIL_PARAM_COMBO_ITERATION_HPP_

#include <citcpp/function_ref.hpp>
#include <vector>

#include "datatypes_config.hpp"
#include "functor_executor.hpp"
#include "internal_model.hpp"
#include "shared_constants.hpp"

namespace citcpp {
namespace detail {

class param_combo_iterator {
  public:
    param_combo_iterator(unsigned int num_params_to_select_from,
                         unsigned int num_params_to_select,
                         const std::vector<unsigned int>& parameter_index_map,
                         bool fixed_last_parameter)
        : num_params_to_select_from_(num_params_to_select_from),
          num_params_to_select_(num_params_to_select),
          parameter_index_map_(parameter_index_map),
          fixed_last_parameter_(fixed_last_parameter),
          param_indices_(num_params_to_select) {}

    unsigned int get_number_of_parameters_to_select_from() const {
      return num_params_to_select_from_;
    }

    unsigned int get_number_of_parameters_to_select() const {
      return num_params_to_select_;
    }

    template <class T_VISITOR>
    void visit_all_parameter_combinations(T_VISITOR& visitor) {
      if (fixed_last_parameter_) {
        const unsigned int real_last_param_idx =
            parameter_index_map_[num_params_to_select_from_ - 1];

        param_indices_[num_params_to_select_ - 1] =
            static_cast<std::uint16_t>(real_last_param_idx);

        if (num_params_to_select_ >= 2) {
          recursively_visit_all_param_combos(
              visitor, static_cast<int>(num_params_to_select_from_ - 2),
              static_cast<int>(num_params_to_select_ - 2));
        } else {
          // We have exactly one parameter to select, which is just the one we
          // have fixed.
          visitor(param_indices_);
        }
      } else {
        recursively_visit_all_param_combos(
            visitor, static_cast<int>(num_params_to_select_from_ - 1),
            static_cast<int>(num_params_to_select_ - 1));
      }
    }

  private:
    template <class T_VISITOR>
    bool recursively_visit_all_param_combos(T_VISITOR& visitor,
                                            int start_idx_for_next,
                                            int current_level) {

      bool cont = true;
      for (int j = start_idx_for_next; j >= current_level; --j) {
        param_indices_[current_level] =
            static_cast<std::uint16_t>(parameter_index_map_[j]);

        if (current_level == 0) {
          cont = visitor(param_indices_);
        } else {
          cont = recursively_visit_all_param_combos(visitor, j - 1,
                                                    current_level - 1);
        }

        if (!cont) {
          break;
        }
      }

      return cont;
    }

  private:
    unsigned int num_params_to_select_from_;
    unsigned int num_params_to_select_;
    const std::vector<unsigned int>& parameter_index_map_;
    bool fixed_last_parameter_;
    param_vector param_indices_;
};

template <typename F, conc_is_void_functor_executor T_EXEC>
class param_combo_functor_parallel_iterator {
  public:
    param_combo_functor_parallel_iterator()
        : iterate_tasks_(), exec_(nullptr) {}

    template <typename... Args>
    param_combo_functor_parallel_iterator(
        unsigned int num_params_to_select_from,
        unsigned int num_params_to_select,
        const std::vector<unsigned int>& parameter_index_map,
        bool fixed_last_parameter, T_EXEC& exec, Args&&... args)
        : iterate_tasks_(), exec_(&exec) {

      const int int_num_params_to_select_from =
          static_cast<int>(num_params_to_select_from);
      const int int_num_params_to_select =
          static_cast<int>(num_params_to_select);

      param_vector param_indices(int_num_params_to_select);
      param_indices[int_num_params_to_select - 1] = static_cast<std::uint16_t>(
          parameter_index_map[int_num_params_to_select_from - 1]);

      if (fixed_last_parameter) {
        if (int_num_params_to_select >= 2) {
          const int loop_ub = int_num_params_to_select_from - 2;
          const int loop_lb = int_num_params_to_select - 2;
          iterate_tasks_.reserve(loop_ub - loop_lb + 1);

          for (int j = loop_ub; j >= loop_lb; --j) {
            iterate_tasks_.emplace_back(parameter_index_map, j, j,
                                        int_num_params_to_select - 1,
                                        param_vector(param_indices), args...);
          }
        } else {
          // If we have fixed the last parameter and shall only select one,
          // then we have to treat that case differently.
          iterate_tasks_.emplace_back(
              parameter_index_map, int_num_params_to_select_from - 1,
              int_num_params_to_select_from - 1, int_num_params_to_select,
              param_vector(param_indices), args...);
        }
      } else {
        const int loop_ub = int_num_params_to_select_from - 1;
        const int loop_lb = int_num_params_to_select - 1;
        iterate_tasks_.reserve(loop_ub - loop_lb + 1);

        for (int j = loop_ub; j >= loop_lb; --j) {
          iterate_tasks_.emplace_back(parameter_index_map, j, j,
                                      int_num_params_to_select,
                                      param_vector(param_indices), args...);
        }
      }
    }

    ~param_combo_functor_parallel_iterator() = default;

    std::size_t get_num_workers() const { return exec_->get_num_workers(); }

    std::size_t get_worker_id() const { return exec_->get_worker_id(); }

    void visit_all_parameter_combinations() {
      auto exec_scope(exec_->create_execution_scope());
      exec_scope.spawn_execution(iterate_tasks_);
    }

    template <typename T_VISITOR>
    void visit_all_functors(T_VISITOR&& visitor) {
      for (auto& t : iterate_tasks_) {
        visitor(t.get_functor());
      }
    }

  private:
    class alignas(false_sharing_avoidance_alignment) iterate_task {
      public:
        iterate_task()
            : func_(),
              param_indices_(),
              parameter_index_map_(nullptr),
              start_idx_(0),
              end_idx_(0),
              num_params_to_select_(0) {}

        template <typename... Args>
        iterate_task(const std::vector<unsigned int>& parameter_index_map,
                     int start_idx, int end_idx, int num_params_to_select,
                     param_vector&& param_indices, Args... args)
            : func_(std::forward<Args>(args)...),
              param_indices_(std::move(param_indices)),
              parameter_index_map_(&parameter_index_map),
              start_idx_(start_idx),
              end_idx_(end_idx),
              num_params_to_select_(num_params_to_select) {}

        void operator()() {
          const int current_level = num_params_to_select_ - 1;
          bool cont = true;
          for (int j = start_idx_; j >= end_idx_; --j) {
            param_indices_[current_level] =
                static_cast<std::uint16_t>((*parameter_index_map_)[j]);

            if (current_level == 0) {
              cont = func_(param_indices_);
            } else {
              cont =
                  recursively_visit_all_param_combos(j - 1, current_level - 1);
            }

            if (!cont) {
              break;
            }
          }
        }

        F& get_functor() { return func_; }
        const F& get_functor() const { return func_; }

      private:
        bool recursively_visit_all_param_combos(int start_idx,
                                                int current_level) {

          bool cont = true;
          for (int j = start_idx; j >= current_level; --j) {
            param_indices_[current_level] =
                static_cast<std::uint16_t>((*parameter_index_map_)[j]);

            if (current_level == 0) {
              cont = func_(param_indices_);
            } else {
              cont =
                  recursively_visit_all_param_combos(j - 1, current_level - 1);
            }

            if (!cont) {
              break;
            }
          }

          return cont;
        }

      private:
        F func_;
        param_vector param_indices_;
        const std::vector<unsigned int>* parameter_index_map_;
        int start_idx_;
        int end_idx_;
        int num_params_to_select_;
    };

  private:
    thread_local_vector<iterate_task> iterate_tasks_;
    T_EXEC* exec_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_PARAM_COMBO_ITERATION_HPP_ */
