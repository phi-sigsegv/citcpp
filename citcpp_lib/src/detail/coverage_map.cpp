#include "coverage_map.hpp"

#include <bit>
#include <stdexcept>

namespace {

// The 64 bits at positions [pos, pos + 64) of the bit array (which has one
// extra word at the end).
inline std::uint64_t load_bits(const std::vector<std::uint64_t>& bits,
                               std::uint64_t pos) {
  const std::size_t w = static_cast<std::size_t>(pos >> 6);
  const unsigned int s = static_cast<unsigned int>(pos & 63);
  return (bits[w] >> s) | ((bits[w + 1] << 1) << (63 - s));
}

// Sets the bits at positions pos + k for every bit k set in `mask`.
inline void or_bits(std::vector<std::uint64_t>& bits, std::uint64_t pos,
                    std::uint64_t mask) {
  const std::size_t w = static_cast<std::size_t>(pos >> 6);
  const unsigned int s = static_cast<unsigned int>(pos & 63);
  bits[w] |= mask << s;
  bits[w + 1] |= (mask >> 1) >> (63 - s);
}

// Mask of the valid bits of the word at offset `off` of a row of the given
// length.
inline std::uint64_t row_word_mask(std::uint64_t length, std::uint64_t off) {
  return (length - off < 64) ? (std::uint64_t(1) << (length - off)) - 1
                             : ~std::uint64_t(0);
}

}  // namespace

namespace citcpp {
namespace detail {

ipog_coverage_map::ipog_coverage_map(
    unsigned int n, unsigned int t, const internal_model& model,
    const std::vector<unsigned int>& parameter_index_map,
    const binom_coeff_table& binomial_coeffs, bool fixed_last_parameter)
    : model_(&model),
      parameter_index_map_(&parameter_index_map),
      binomial_coeffs_(&binomial_coeffs),
      n_(n),
      t_(t),
      storage_(std::make_unique<ipog_coverage_map_storage>()) {

  if (!fixed_last_parameter || t == 0 || n < t) {
    throw std::logic_error(
        "ipog_coverage_map: requires a fixed last parameter and 1 <= t <= n");
  }

  const auto& num_values = model.get_parameter_num_values();
  const unsigned int s = t - 1;  // number of earlier parameters per tuple
  const unsigned int m = n - 1;  // number of earlier positions
  last_domain_size_ = num_values[parameter_index_map[m]];

  E_.assign(s + 1, std::vector<std::uint64_t>(m + 1, 0));
  for (unsigned int j = 0; j <= m; ++j) {
    E_[0][j] = 1;
  }
  for (unsigned int k = 1; k <= s; ++k) {
    for (unsigned int j = 0; j < m; ++j) {
      E_[k][j + 1] =
          E_[k][j] + num_values[parameter_index_map[j]] * E_[k - 1][j];
    }
  }

  storage_->tuples_per_value = E_[s][m];
  total_num_tuples_ = last_domain_size_ * storage_->tuples_per_value;
  const std::size_t num_words =
      static_cast<std::size_t>((total_num_tuples_ + 63) / 64) + 1;
  storage_->covered.assign(num_words, 0);
  storage_->valid.assign(num_words, 0);

  if (s >= 2) {
    rank_terms_.assign(s, std::vector<long long>(m + 1, 0));
    for (unsigned int l = 0; l < s; ++l) {
      for (unsigned int x = 0; x <= m; ++x) {
        rank_terms_[l][x] = binomial_coeffs.get_coefficient(x, l + 1);
      }
    }
  }

  if (s >= 1) {
    slot_position_.resize(E_[1][m]);
    for (unsigned int q = 0; q < m; ++q) {
      for (std::uint64_t slot = E_[1][q]; slot < E_[1][q + 1]; ++slot) {
        slot_position_[slot] = q;
      }
    }
  }

  // Enumerate the combinations in the order of the original implementation:
  // the positions p[s-1] > ... > p[0] of the earlier parameters are chosen
  // from the top level down, each level descending.
  const std::size_t num_combos = static_cast<std::size_t>(
      s == 0 ? 1 : binomial_coeffs.get_coefficient(m, s));
  combos_.reserve(num_combos);
  storage_->combo_domain_sizes.reserve(num_combos * s);
  storage_->combo_value_weights.reserve(num_combos * s);

  std::vector<unsigned int> p(s);
  auto emit = [&]() {
    second_level_type combo;
    combo.storage_ = storage_.get();
    combo.param_indices_ = param_vector(s + 1);
    combo.first_element_ = storage_->combo_domain_sizes.size();
    combo.index_ = static_cast<second_level_type::size_type>(combos_.size());
    combo.last_domain_size_ = last_domain_size_;
    std::uint64_t size = last_domain_size_;
    long long rank = 0;
    for (unsigned int l = 0; l < s; ++l) {
      const unsigned int q = p[l];
      const unsigned int d = num_values[parameter_index_map[q]];
      combo.param_indices_[l] =
          static_cast<std::uint16_t>(parameter_index_map[q]);
      combo.base_ += E_[l + 1][q];
      storage_->combo_domain_sizes.push_back(d);
      storage_->combo_value_weights.push_back(E_[l][q]);
      size *= d;
      const unsigned int top = (l + 1 == s) ? m - 1 : p[l + 1] - 1;
      rank += binomial_coeffs.get_coefficient(top + 1, l + 1) -
              binomial_coeffs.get_coefficient(q + 1, l + 1);
    }
    combo.param_indices_[s] =
        static_cast<std::uint16_t>(parameter_index_map[m]);
    combo.size_ = static_cast<second_level_type::size_type>(size);
    // The kernels find the combination of a tuple by this rank; it has to
    // agree with the enumeration order.
    if (rank != static_cast<long long>(combos_.size())) {
      throw std::logic_error("ipog_coverage_map: inconsistent combination rank");
    }
    combos_.push_back(std::move(combo));
  };
  storage_->num_covered.assign(num_combos, 0);

  if (s == 0) {
    emit();
  } else {
    auto recurse = [&](auto& self, int level, int start) -> void {
      for (int j = start; j >= level; --j) {
        p[level] = static_cast<unsigned int>(j);
        if (level == 0) {
          emit();
        } else {
          self(self, level - 1, j - 1);
        }
      }
    };
    recurse(recurse, static_cast<int>(s) - 1, static_cast<int>(m) - 1);
  }
}

void ipog_coverage_map::load_test(const test& t) const {
  const unsigned int m = n_ - 1;
  const std::vector<unsigned int>& pim = *parameter_index_map_;
  test_values_.resize(m);
  assigned_positions_.clear();
  test_slots_.assign(static_cast<std::size_t>((E_[1][m] + 63) / 64), 0);
  for (unsigned int q = 0; q < m; ++q) {
    const int value = t.get_values()[pim[q]];
    test_values_[q] = value;
    if (value >= 0) {
      assigned_positions_.push_back(q);
      const std::uint64_t slot = E_[1][q] + static_cast<std::uint64_t>(value);
      test_slots_[slot >> 6] |= std::uint64_t(1) << (slot & 63);
    }
  }
}

template <bool with_rank, typename F>
void ipog_coverage_map::for_each_row(F&& f) const {
  const unsigned int s = t_ - 1;
  const unsigned int m = n_ - 1;
  if (s == 1) {
    // A single row over all earlier slots; the combination of the slot of
    // position i is the one with index (m - 1) - i.
    f(std::uint64_t(0), E_[1][m], static_cast<long long>(m) - 1);
    return;
  }
  // s >= 2: the rank of a combination with positions p[0] < ... < p[s-1] is
  // sum_l [ C(top_l + 1, l + 1) - C(p[l] + 1, l + 1) ] with top_{s-1} = m - 1
  // and top_l = p[l + 1] - 1. The levels >= 1 are fixed per row, the level 0
  // term is (p[1] - 1) - p[0].
  auto walk = [&](auto& self, unsigned int level, std::size_t candidates,
                  unsigned int top, std::uint64_t base,
                  long long rank) -> void {
    for (std::size_t c = candidates; c-- > 0;) {
      const unsigned int q = assigned_positions_[c];
      if (q < level) {
        break;  // Not enough positions left below for the lower levels.
      }
      const std::uint64_t value = static_cast<std::uint64_t>(test_values_[q]);
      const std::uint64_t level_base =
          base + E_[level + 1][q] + value * E_[level][q];
      long long level_rank = 0;
      if constexpr (with_rank) {
        const std::vector<long long>& terms = rank_terms_[level];
        level_rank = rank + terms[top + 1] - terms[q + 1];
      }
      if (level == 1) {
        f(level_base, E_[1][q], level_rank + static_cast<long long>(q) - 1);
      } else {
        self(self, level - 1, c, q - 1, level_base, level_rank);
      }
    }
  };
  walk(walk, s - 1, assigned_positions_.size(), m - 1, 0, 0);
}

void ipog_coverage_map::add_gains(
    const test& t, const bitset_uint64& valid_values,
    std::vector<unsigned long long>& gain_per_value) const {

  const std::vector<std::uint64_t>& covered = storage_->covered;
  const std::uint64_t tuples_per_value = storage_->tuples_per_value;

  if (t_ == 1) {
    for (unsigned int v = 0; v < last_domain_size_; ++v) {
      if (valid_values.test(v) && !((covered[v >> 6] >> (v & 63)) & 1)) {
        gain_per_value[v] += 1;
      }
    }
    return;
  }

  load_test(t);
  for_each_row<false>([&](std::uint64_t base, std::uint64_t length,
                          long long) {
    for (unsigned int v = 0; v < last_domain_size_; ++v) {
      if (!valid_values.test(v)) {
        continue;
      }
      const std::uint64_t row = base + v * tuples_per_value;
      unsigned long long gain = 0;
      for (std::uint64_t w = 0, off = 0; off < length; ++w, off += 64) {
        const std::uint64_t uncovered_in_test = ~load_bits(covered, row + off) &
                                                test_slots_[w] &
                                                row_word_mask(length, off);
        gain += static_cast<unsigned long long>(std::popcount(uncovered_in_test));
      }
      gain_per_value[v] += gain;
    }
  });
}

unsigned long long ipog_coverage_map::cover(const test& t, int last_value,
                                            const bitset_uint64& valid_values) {
  std::vector<std::uint64_t>& covered = storage_->covered;
  std::vector<std::uint64_t>& valid = storage_->valid;
  const std::uint64_t tuples_per_value = storage_->tuples_per_value;
  const std::uint64_t c = static_cast<std::uint64_t>(last_value);

  if (t_ == 1) {
    unsigned long long newly = 0;
    if (!((covered[c >> 6] >> (c & 63)) & 1)) {
      covered[c >> 6] |= std::uint64_t(1) << (c & 63);
      ++storage_->num_covered[0];
      newly = 1;
    }
    for (unsigned int v = 0; v < last_domain_size_; ++v) {
      if (valid_values.test(v)) {
        valid[v >> 6] |= std::uint64_t(1) << (v & 63);
      }
    }
    return newly;
  }

  load_test(t);
  std::uint32_t* const num_covered = storage_->num_covered.data();
  unsigned long long newly = 0;
  for_each_row<true>([&](std::uint64_t base, std::uint64_t length,
                         long long rank_base) {
    const std::uint64_t row = base + c * tuples_per_value;
    for (std::uint64_t w = 0, off = 0; off < length; ++w, off += 64) {
      const std::uint64_t bits = test_slots_[w] & row_word_mask(length, off);
      if (!bits) {
        continue;
      }
      std::uint64_t new_bits = bits & ~load_bits(covered, row + off);
      if (new_bits) {
        or_bits(covered, row + off, new_bits);
        newly += static_cast<unsigned long long>(std::popcount(new_bits));
        // Account the newly covered tuples to their combinations.
        for (; new_bits; new_bits &= new_bits - 1) {
          const std::uint64_t slot =
              off + static_cast<std::uint64_t>(std::countr_zero(new_bits));
          ++num_covered[static_cast<std::size_t>(
              rank_base - static_cast<long long>(slot_position_[slot]))];
        }
      }
      for (unsigned int v = 0; v < last_domain_size_; ++v) {
        if (valid_values.test(v)) {
          or_bits(valid, base + v * tuples_per_value + off, bits);
        }
      }
    }
  });
  return newly;
}

}  // namespace detail
}  // namespace citcpp
