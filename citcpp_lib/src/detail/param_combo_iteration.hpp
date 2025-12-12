#ifndef DETAIL_PARAM_COMBO_ITERATION_HPP_
#define DETAIL_PARAM_COMBO_ITERATION_HPP_

#include <vector>

#include "datatypes_config.hpp"
#include "function_ref.hpp"
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
          parameter_index_map_(&parameter_index_map),
          fixed_last_parameter_(fixed_last_parameter),
          param_indices_(num_params_to_select) {}

    param_combo_iterator(const param_combo_iterator& other) = default;
    param_combo_iterator(param_combo_iterator&& other) = default;

    ~param_combo_iterator() = default;

    param_combo_iterator& operator=(const param_combo_iterator& other) =
        default;
    param_combo_iterator& operator=(param_combo_iterator&& other) = default;

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
            (*parameter_index_map_)[num_params_to_select_from_ - 1];

        param_indices_[num_params_to_select_ - 1] = real_last_param_idx;

        if (num_params_to_select_ >= 2) {
          recursively_visit_all_param_combos(visitor,
                                             num_params_to_select_from_ - 2,
                                             num_params_to_select_ - 2);
        } else {
          // We have exactly one parameter to select, which is just the one we
          // have fixed.
          visitor(param_indices_);
        }
      } else {
        recursively_visit_all_param_combos(
            visitor, num_params_to_select_from_ - 1, num_params_to_select_ - 1);
      }
    }

  private:
    template <class T_VISITOR>
    bool recursively_visit_all_param_combos(T_VISITOR& visitor,
                                            int start_idx_for_next,
                                            int current_level) {

      bool cont = true;
      for (int j = start_idx_for_next; j >= current_level; --j) {
        param_indices_[current_level] = (*parameter_index_map_)[j];

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
    const std::vector<unsigned int>* parameter_index_map_;
    bool fixed_last_parameter_;
    param_vector param_indices_;
};

class param_combo_parallel_iterator {
  public:
    param_combo_parallel_iterator() = default;

    param_combo_parallel_iterator(
        unsigned int num_params_to_select_from,
        unsigned int num_params_to_select,
        const std::vector<unsigned int>& parameter_index_map,
        bool fixed_last_parameter, thread_pool& tp)
        : num_params_to_select_from_(num_params_to_select_from),
          num_params_to_select_(num_params_to_select),
          parameter_index_map_(&parameter_index_map),
          fixed_last_parameter_(fixed_last_parameter),
          param_indices_(num_params_to_select),
          tp_(&tp),
          iterate_tasks_(),
          visitor_() {

      param_indices_[num_params_to_select - 1] =
          parameter_index_map[num_params_to_select_from - 1];

      if (fixed_last_parameter) {
        // If we have fixed the last parameter and shall only select one,
        // then there is nothing to do.
        if (num_params_to_select >= 2) {
          for (unsigned int j = num_params_to_select_from - 2;
               j >= (num_params_to_select - 2); --j) {

            iterate_tasks_.emplace_back(this, j, j, num_params_to_select - 1,
                                        param_vector(param_indices_));
          }
        }
      } else {
        for (unsigned int j = num_params_to_select_from - 1;
             j >= (num_params_to_select - 1); --j) {

          iterate_tasks_.emplace_back(this, j, j, num_params_to_select,
                                      param_vector(param_indices_));
        }
      }
    }

    param_combo_parallel_iterator(const param_combo_parallel_iterator& other)
        : num_params_to_select_from_(other.num_params_to_select_from_),
          num_params_to_select_(other.num_params_to_select_),
          parameter_index_map_(other.parameter_index_map_),
          fixed_last_parameter_(other.fixed_last_parameter_),
          param_indices_(other.param_indices_),
          tp_(other.tp_),
          iterate_tasks_(other.iterate_tasks_),
          visitor_(other.visitor_) {

      for (auto& task : iterate_tasks_) {
        task.set_iterator(this);
      }
    }

    param_combo_parallel_iterator(param_combo_parallel_iterator&& other)
        : num_params_to_select_from_(other.num_params_to_select_from_),
          num_params_to_select_(other.num_params_to_select_),
          parameter_index_map_(other.parameter_index_map_),
          fixed_last_parameter_(other.fixed_last_parameter_),
          param_indices_(std::move(other.param_indices_)),
          tp_(other.tp_),
          iterate_tasks_(std::move(other.iterate_tasks_)),
          visitor_(std::move(other.visitor_)) {

      for (auto& task : iterate_tasks_) {
        task.set_iterator(this);
      }
    }

    ~param_combo_parallel_iterator() = default;

    param_combo_parallel_iterator& operator=(
        const param_combo_parallel_iterator& other) {

      if (this != &other) {
        num_params_to_select_from_ = other.num_params_to_select_from_;
        num_params_to_select_ = other.num_params_to_select_;
        parameter_index_map_ = other.parameter_index_map_;
        fixed_last_parameter_ = other.fixed_last_parameter_;
        param_indices_ = other.param_indices_;
        tp_ = other.tp_;
        iterate_tasks_ = other.iterate_tasks_;
        visitor_ = other.visitor_;

        for (auto& task : iterate_tasks_) {
          task.set_iterator(this);
        }
      }

      return *this;
    }

    param_combo_parallel_iterator& operator=(
        param_combo_parallel_iterator&& other) {

      if (this != &other) {
        num_params_to_select_from_ = other.num_params_to_select_from_;
        num_params_to_select_ = other.num_params_to_select_;
        parameter_index_map_ = other.parameter_index_map_;
        fixed_last_parameter_ = other.fixed_last_parameter_;
        param_indices_ = std::move(other.param_indices_);
        tp_ = other.tp_;
        iterate_tasks_ = std::move(other.iterate_tasks_);
        visitor_ = std::move(other.visitor_);

        for (auto& task : iterate_tasks_) {
          task.set_iterator(this);
        }
      }

      return *this;
    }

    unsigned int get_number_of_parameters_to_select_from() const {
      return num_params_to_select_from_;
    }

    unsigned int get_number_of_parameters_to_select() const {
      return num_params_to_select_;
    }

    unsigned int get_num_workers() const { return tp_->get_num_workers(); }

    unsigned int get_worker_id() const { return tp_->get_worker_id(); }

    template <class T_VISITOR>
    void visit_all_parameter_combinations(T_VISITOR& visitor) {
      visitor_ = visitor;

      if (fixed_last_parameter_ && num_params_to_select_ <= 1) {
        // We have exactly one parameter to select, which is just the one we
        // have fixed.
        visitor(param_indices_);
      } else {
        task_group tg(tp_->createTaskGroup());
        for (unsigned int i = 1; i < iterate_tasks_.size(); ++i) {
          iterate_task& task = iterate_tasks_[i];
          task.reset();
          tg.spawn(i, &task);
        }
        iterate_task& task = iterate_tasks_[0];
        task.reset();
        tg.spawn_and_wait(&task);
      }
    }

  private:
    class alignas(false_sharing_avoidance_alignment) iterate_task
        : public functor_task_base<iterate_task> {
      private:
        typedef functor_task_base<iterate_task> base_type;
        typedef iterate_task this_type;

      public:
        iterate_task() = default;

        iterate_task(param_combo_parallel_iterator* iterator, int start_idx,
                     int end_idx, int num_params_to_select,
                     param_vector&& param_indices)
            : base_type(),
              iterator_(iterator),
              start_idx_(start_idx),
              end_idx_(end_idx),
              num_params_to_select_(num_params_to_select),
              param_indices_(std::move(param_indices)) {}

        virtual ~iterate_task() {}

        void operator()() {
          const int current_level = num_params_to_select_ - 1;
          bool cont = true;
          for (int j = start_idx_; j >= end_idx_; --j) {
            param_indices_[current_level] =
                (*iterator_->parameter_index_map_)[j];

            if (current_level == 0) {
              cont = iterator_->visitor_(param_indices_);
            } else {
              cont = iterator_->recursively_visit_all_param_combos(
                  param_indices_, j - 1, current_level - 1);
            }

            if (!cont) {
              break;
            }
          }
        }

        void set_iterator(param_combo_parallel_iterator* iterator) {
          iterator_ = iterator;
        }

      private:
        param_combo_parallel_iterator* iterator_;
        int start_idx_;
        int end_idx_;
        int num_params_to_select_;
        param_vector param_indices_;
    };

    friend class iterate_task;

    bool recursively_visit_all_param_combos(param_vector& param_indices,
                                            int start_idx, int current_level) {

      bool cont = true;
      for (int j = start_idx; j >= current_level; --j) {
        param_indices[current_level] = (*parameter_index_map_)[j];

        if (current_level == 0) {
          cont = visitor_(param_indices);
        } else {
          cont = recursively_visit_all_param_combos(param_indices, j - 1,
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
    const std::vector<unsigned int>* parameter_index_map_;
    bool fixed_last_parameter_;
    param_vector param_indices_;
    thread_pool* tp_;
    std::vector<iterate_task> iterate_tasks_;
    function_ref<bool(const param_vector&)> visitor_;
};

}  // namespace detail
}  // namespace citcpp

#endif /* DETAIL_PARAM_COMBO_ITERATION_HPP_ */
