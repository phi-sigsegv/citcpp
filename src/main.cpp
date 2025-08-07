#include <CLI11.hpp>
#include <atomic>
#include <chrono>
#include <citcpp/citcpp.hpp>
#include <csignal>
#include <iomanip>
#include <ios>
#include <iostream>
#include <thread>

citcpp::input_model create_large_model() {
  using namespace citcpp;

  input_model model;

  for (int p_idx = 0; p_idx < 120; ++p_idx) {
    parameter p;
    p.get_values().push_back(std::to_string(1));
    model.get_parameters().push_back(p);
  }
  for (int p_idx = 0; p_idx < 43; ++p_idx) {
    parameter p;
    for (int v = 1; v <= 2; ++v) {
      p.get_values().push_back(std::to_string(v));
    }
    model.get_parameters().push_back(p);
  }
  for (int p_idx = 0; p_idx < 16; ++p_idx) {
    parameter p;
    for (int v = 1; v <= 3; ++v) {
      p.get_values().push_back(std::to_string(v));
    }
    model.get_parameters().push_back(p);
  }
  for (int p_idx = 0; p_idx < 21; ++p_idx) {
    parameter p;
    for (int v = 1; v <= 4; ++v) {
      p.get_values().push_back(std::to_string(v));
    }
    model.get_parameters().push_back(p);
  }
  for (int p_idx = 0; p_idx < 13; ++p_idx) {
    parameter p;
    for (int v = 1; v <= 5; ++v) {
      p.get_values().push_back(std::to_string(v));
    }
    model.get_parameters().push_back(p);
  }
  for (int p_idx = 0; p_idx < 4; ++p_idx) {
    parameter p;
    for (int v = 1; v <= 6; ++v) {
      p.get_values().push_back(std::to_string(v));
    }
    model.get_parameters().push_back(p);
  }
  for (int p_idx = 0; p_idx < 1; ++p_idx) {
    parameter p;
    for (int v = 1; v <= 7; ++v) {
      p.get_values().push_back(std::to_string(v));
    }
    model.get_parameters().push_back(p);
  }
  for (int p_idx = 0; p_idx < 2; ++p_idx) {
    parameter p;
    for (int v = 1; v <= 8; ++v) {
      p.get_values().push_back(std::to_string(v));
    }
    model.get_parameters().push_back(p);
  }
  for (int p_idx = 0; p_idx < 1; ++p_idx) {
    parameter p;
    for (int v = 1; v <= 9; ++v) {
      p.get_values().push_back(std::to_string(v));
    }
    model.get_parameters().push_back(p);
  }
  for (int p_idx = 0; p_idx < 1; ++p_idx) {
    parameter p;
    for (int v = 1; v <= 10; ++v) {
      p.get_values().push_back(std::to_string(v));
    }
    model.get_parameters().push_back(p);
  }
  for (int p_idx = 0; p_idx < 1; ++p_idx) {
    parameter p;
    for (int v = 1; v <= 11; ++v) {
      p.get_values().push_back(std::to_string(v));
    }
    model.get_parameters().push_back(p);
  }
  for (int p_idx = 0; p_idx < 1; ++p_idx) {
    parameter p;
    for (int v = 1; v <= 12; ++v) {
      p.get_values().push_back(std::to_string(v));
    }
    model.get_parameters().push_back(p);
  }
  for (int p_idx = 0; p_idx < 2; ++p_idx) {
    parameter p;
    for (int v = 1; v <= 15; ++v) {
      p.get_values().push_back(std::to_string(v));
    }
    model.get_parameters().push_back(p);
  }
  for (int p_idx = 0; p_idx < 1; ++p_idx) {
    parameter p;
    for (int v = 1; v <= 16; ++v) {
      p.get_values().push_back(std::to_string(v));
    }
    model.get_parameters().push_back(p);
  }

  return model;
}

citcpp::input_model create_medium_model() {
  using namespace citcpp;

  input_model model;

  for (int p_idx = 0; p_idx < 100; ++p_idx) {
    parameter p;
    for (int v = 1; v <= 2; ++v) {
      p.get_values().push_back(std::to_string(v));
    }
    model.get_parameters().push_back(p);
  }

  return model;
}

citcpp::input_model create_pict_example_model() {
  using namespace citcpp;

  input_model model;

  model.add_parameter(
      parameter().name("PLATFORM").values({{"x86"}, {"x64"}, {"arm"}}));
  model.add_parameter(parameter().name("CPUS").values({{"1"}, {"2"}, {"4"}}));
  model.add_parameter(
      parameter().name("RAM").values({{"1GB"}, {"4GB"}, {"64GB"}}));
  model.add_parameter(parameter().name("HDD").values({{"SCSI"}, {"IDE"}}));
  model.add_parameter(
      parameter().name("OS").values({{"Win7"}, {"Win8"}, {"Win10"}}));
  model.add_parameter(parameter().name("Browser").values(
      {{"Edge"}, {"Opera"}, {"Chrome"}, {"Firefox"}}));
  model.add_parameter(
      parameter().name("APP").values({{"Word"}, {"Excel"}, {"Powerpoint"}}));

  return model;
}

namespace {

volatile std::sig_atomic_t g_signal_status;

class InteractionStrengthValidator : public CLI::Validator {
  public:
    InteractionStrengthValidator() : Validator() {
      func_ = [](const std::string &input) {
        using CLI::detail::lexical_cast;

        int val;
        const bool converted = lexical_cast(input, val);

        if ((!converted) || (val < -1 || val == 0)) {
          std::stringstream out;
          out << "Value " << input << " neither >= 1, nor -1";
          return out.str();
        }

        return std::string{};
      };
    }
};

}  // namespace

void signal_handler(int signal) { g_signal_status = signal; }

int main(int argc, char *argv[]) {
  using namespace citcpp;
  using namespace std::chrono_literals;

  CLI::App app{
      "This is citcpp, a tool for combinatorial testing, which can be used for "
      "generating covering arrays and measuring t-way coverage of a given "
      "testset."};

  int interaction_strength = 2;
  app.add_option("--doi", interaction_strength,
                 "Specifies the degree of interactions to be covered. Use -1 "
                 "for mixed strength as specified in the input file. The "
                 "default value is 2.")
      ->check(InteractionStrengthValidator());

  bool show_progress = false;
  app.add_flag("--progress", show_progress,
               "Set this flag to enable displaying progress information.");

  std::string sep{","};
  app.add_option("--sep", sep,
                 "Set this to specify the separator for values in a testset. "
                 "The default value is \",\".");

  bool parallel = false;
  app.add_flag(
      "--par", parallel,
      "Set this flag to enable parallelization of the algorithm execution.");

  CLI::App *command_cagen =
      app.add_subcommand("cagen", "This command generates a covering array.");

  bool rand_star = true;
  command_cagen->add_option(
      "--randstar", rand_star,
      "Set this to \"on\" to randomize don't care values and to \"off\" to "
      "not randomize don't care values, i.e. to keep them in the generated "
      "testset. The default value is \"on\".");

  CLI::App *command_cov_measure = app.add_subcommand(
      "covm", "This command measures the coverage of a given testset.");

  app.require_subcommand(1, 1);

  CLI11_PARSE(app, argc, argv);

  input_model model(create_pict_example_model());

  std::cout << "Starting execution\n" << std::endl;
  std::unique_ptr<exec_handle_ipog> handle =
      compute_covering_array_ipog(model, interaction_strength,
                                  covering_array_computation_config()
                                      .with_replace_dont_care_values(rand_star)
                                      .with_multithreading_enabled(parallel));

  // Install a signal handler allowing to gracefully abort the computation using
  // Ctrl+C.
  std::signal(SIGINT, signal_handler);

  const auto default_precision{std::cout.precision()};
  std::cout << std::setprecision(1);
  std::cout << std::fixed;

  auto f = handle->get_test_set();
  while (f.wait_for(1s) == std::future_status::timeout) {
    if (show_progress) {
      unsigned long long num_covered_combos =
          handle->get_number_of_covered_combinations();
      unsigned long long num_combos_to_cover =
          handle->get_number_of_combinations_to_cover();
      double precent_done =
          (double)num_covered_combos / (double)num_combos_to_cover * 100.0;
      std::cout << "\r";
      std::cout << "combos: (" << num_covered_combos << " / "
                << num_combos_to_cover << ") " << precent_done << "%, params: ("
                << handle->get_number_of_processed_parameters() << " / "
                << model.get_parameters().size() << "), "
                << handle->get_testset_size() << " tests" << std::flush;
    }

    if (g_signal_status == SIGINT) {
      handle->abort();
    }
  }

  unsigned long long num_covered_combos =
      handle->get_number_of_covered_combinations();
  unsigned long long num_combos_to_cover =
      handle->get_number_of_combinations_to_cover();
  double precent_done =
      (double)num_covered_combos / (double)num_combos_to_cover * 100.0;

  std::cout << "\r";
  std::cout << "combos: (" << num_covered_combos << " / " << num_combos_to_cover
            << ") " << precent_done << "%, params: ("
            << handle->get_number_of_processed_parameters() << " / "
            << model.get_parameters().size() << "), "
            << handle->get_testset_size() << " tests\n"
            << std::endl;

  // restore defaults in formatting.
  std::cout << std::setprecision(default_precision);
  std::cout << std::defaultfloat;

  test_set t(f.get());

  std::cout << "test set has the following " << t.get_list_of_tests().size()
            << " rows:\n";
  std::cout << t << std::endl;

  const auto duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
  std::cout << "Execution took: " << duration_seconds << std::endl;
}
