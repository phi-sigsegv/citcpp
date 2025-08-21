#include <CLI11.hpp>
#include <atomic>
#include <chrono>
#include <citcpp/citcpp.hpp>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>
#include <thread>

#include "acts_model_parser.hpp"
#include "config.hpp"
#include "test_set_parser.hpp"

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

int execute_cagen(const std::string &model_file_path,
                  const std::string &test_set_file_path,
                  int interaction_strength, bool show_progress,
                  const std::string &sep, bool parallel, bool rand_star) {

  using namespace citcpp;
  using namespace citcpp::detail;
  using namespace std::chrono_literals;

  // We read the ACTS model file into a string.
  std::ifstream model_file_is{model_file_path};
  if (!model_file_is.is_open()) {
    std::cerr << "Cannot open model file " << model_file_path << " for reading"
              << std::endl;

    return 1;
  }

  // Ensure early that we can write to the output file.
  std::ofstream test_set_file_os{test_set_file_path};
  if (!test_set_file_os.is_open()) {
    std::cerr << "Cannot open testset file " << test_set_file_path
              << " for writing" << std::endl;

    return 1;
  }

  std::ostringstream model_file_oss{};
  model_file_oss << model_file_is.rdbuf();

  acts_model_parser acts_parser;
  input_model model;
  if (!acts_parser.parse_input_model(model_file_oss.view(), model)) {
    std::cerr << acts_parser.get_last_error_message() << std::endl;

    return 1;
  }

  std::cout << "System name : " << model.get_name() << "\n" << std::endl;
  std::cout << "Strength    : " << interaction_strength << std::endl;
  std::cout << "Parameters  : " << model.get_parameters().size() << "\n"
            << std::endl;

  std::unique_ptr<cagen_exec_handle_ipog> handle =
      compute_covering_array_ipog(model, interaction_strength,
                                  covering_array_computation_config()
                                      .with_replace_dont_care_values(rand_star)
                                      .with_multithreading_enabled(parallel)
                                      .with_value_separator(sep));

  const auto default_precision{std::cout.precision()};
  std::cout << std::setprecision(1);
  std::cout << std::fixed;

  auto f = handle->get_test_set();
  bool aborted = false;
  while (f.wait_for(1s) == std::future_status::timeout) {
    if (show_progress) {
      unsigned long long num_covered_combos =
          handle->get_number_of_covered_combinations();
      unsigned long long num_combos_to_cover =
          handle->get_number_of_combinations_to_cover();
      double precent_done =
          (double)num_covered_combos / (double)num_combos_to_cover * 100.0;
      std::cout << "\r";
      std::cout << "tuples: (" << num_covered_combos << " / "
                << num_combos_to_cover << ") " << precent_done << "%, params: ("
                << handle->get_number_of_processed_parameters() << " / "
                << model.get_parameters().size() << "), "
                << handle->get_testset_size() << " tests" << std::flush;
    }

    if (g_signal_status == SIGINT) {
      handle->abort();
      aborted = true;
    }
  }

  unsigned long long num_covered_combos =
      handle->get_number_of_covered_combinations();
  unsigned long long num_combos_to_cover =
      handle->get_number_of_combinations_to_cover();
  double precent_done =
      (double)num_covered_combos / (double)num_combos_to_cover * 100.0;

  std::cout << "\r";
  std::cout << "tuples: (" << num_covered_combos << " / " << num_combos_to_cover
            << ") " << precent_done << "%, params: ("
            << handle->get_number_of_processed_parameters() << " / "
            << model.get_parameters().size() << "), "
            << handle->get_testset_size() << " tests\n"
            << std::endl;

  // restore defaults in formatting.
  std::cout << std::setprecision(default_precision);
  std::cout << std::defaultfloat;

  cagen_exec_result result(f.get());

  if (aborted) {
    std::cout << "WARNING: Covering array computation has been aborted. The "
                 "generated testset is most likely incomplete regarding the "
                 "specified coverage."
              << std::endl;
  }

  const auto duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
  std::cout << "Execution took: " << duration_seconds << std::endl;

  test_set_file_os << result.get_result() << std::endl;
  std::cout << "Output file: " << test_set_file_path << std::endl;

  return 0;
}

int execute_covm(const std::string &model_file_path,
                 const std::string &test_set_file_path,
                 const std::string &coverage_measurement_file_path,
                 int interaction_strength, bool show_progress,
                 const std::string &sep, bool parallel) {

  using namespace citcpp;
  using namespace citcpp::detail;
  using namespace std::chrono_literals;

  // We read the ACTS model file into a string.
  std::ifstream model_file_is{model_file_path};
  if (!model_file_is.is_open()) {
    std::cerr << "Cannot open model file " << model_file_path << " for reading"
              << std::endl;

    return 1;
  }

  // We read the testset file into a string.
  std::ifstream test_set_file_is{test_set_file_path};
  if (!test_set_file_is.is_open()) {
    std::cerr << "Cannot open testset file " << test_set_file_path
              << " for reading" << std::endl;

    return 1;
  }

  // Ensure early that we can write to the output file.
  std::ofstream coverage_measurement_file_os{coverage_measurement_file_path};
  if (!coverage_measurement_file_os.is_open()) {
    std::cerr << "Cannot open coverage measurement file "
              << coverage_measurement_file_path << " for writing" << std::endl;

    return 1;
  }

  std::ostringstream model_file_oss{};
  model_file_oss << model_file_is.rdbuf();

  acts_model_parser acts_parser;
  input_model model;
  if (!acts_parser.parse_input_model(model_file_oss.view(), model)) {
    std::cerr << acts_parser.get_last_error_message() << std::endl;

    return 1;
  }

  std::ostringstream test_set_file_oss{};
  test_set_file_oss << test_set_file_is.rdbuf();

  test_set_parser testset_parser(model, sep);
  test_set tests;
  if (!testset_parser.parse_test_set(test_set_file_oss.view(), tests)) {
    std::cerr << testset_parser.get_last_error_message() << std::endl;

    return 1;
  }

  std::cout << "System name : " << model.get_name() << "\n" << std::endl;
  std::cout << "Strength    : " << interaction_strength << std::endl;
  std::cout << "Parameters  : " << model.get_parameters().size() << std::endl;
  std::cout << "Testset size: " << tests.get_list_of_tests().size() << "\n"
            << std::endl;

  std::unique_ptr<covm_exec_handle> handle =
      measure_coverage(model, tests, interaction_strength,
                       coverage_measurement_config()
                           .with_multithreading_enabled(parallel)
                           .with_value_separator(sep));

  const auto default_precision{std::cout.precision()};
  std::cout << std::setprecision(1);
  std::cout << std::fixed;

  auto f = handle->get_coverage_measurement();
  bool aborted = false;
  while (f.wait_for(1s) == std::future_status::timeout) {
    if (show_progress) {
      unsigned long long num_checked_combos =
          handle->get_number_of_checked_combinations();
      unsigned long long num_combos_to_cover =
          handle->get_number_of_combinations_to_cover();
      double precent_done =
          (double)num_checked_combos / (double)num_combos_to_cover * 100.0;
      std::cout << "\r";
      std::cout << "tuples: (" << num_checked_combos << " / "
                << num_combos_to_cover << ") " << precent_done << "%"
                << std::flush;
    }

    if (g_signal_status == SIGINT) {
      handle->abort();
      aborted = true;
    }
  }

  unsigned long long num_checked_combos =
      handle->get_number_of_checked_combinations();
  unsigned long long num_combos_to_cover =
      handle->get_number_of_combinations_to_cover();
  double precent_done =
      (double)num_checked_combos / (double)num_combos_to_cover * 100.0;

  std::cout << "\r";
  std::cout << "tuples: (" << num_checked_combos << " / " << num_combos_to_cover
            << ") " << precent_done << "%\n"
            << std::endl;

  // restore defaults in formatting.
  std::cout << std::setprecision(default_precision);
  std::cout << std::defaultfloat;

  covm_exec_result result(f.get());

  if (aborted) {
    std::cout << "WARNING: Coverage measurement has been aborted. The "
                 "measurement is most likely incomplete, i.e. reporting less "
                 "coverage."
              << std::endl;
  }

  const auto duration_seconds = std::chrono::duration<double>(
      std::chrono::milliseconds(handle->get_duration_in_milli_seconds()));
  std::cout << "Execution took: " << duration_seconds << std::endl;

  coverage_measurement_file_os << result.get_result() << std::endl;
  std::cout << "Output file: " << coverage_measurement_file_path << std::endl;

  return 0;
}

}  // namespace

void signal_handler(int signal) { g_signal_status = signal; }

int main(int argc, char *argv[]) {
  using namespace citcpp;
  using namespace citcpp::detail;
  using namespace std::chrono_literals;

  CLI::App app{
      "This is citcpp, a tool for combinatorial testing, which can be used for "
      "generating covering arrays and measuring t-way coverage of a given "
      "testset."};

  bool show_version = false;
  app.add_flag("-v,--version", show_version,
               "Print version information and exit.");

  CLI::App *command_cagen =
      app.add_subcommand("cagen", "This command generates a covering array.");

  int interaction_strength = 2;
  command_cagen
      ->add_option("--doi", interaction_strength,
                   "Specifies the degree of interactions to be covered. Use -1 "
                   "for mixed strength as specified in the input file. The "
                   "default value is 2.")
      ->check(InteractionStrengthValidator());

  bool show_progress = false;
  command_cagen->add_flag(
      "--progress", show_progress,
      "Set this flag to enable displaying progress information.");

  std::string sep{covering_array_computation_config().value_separator()};
  command_cagen->add_option(
      "--sep", sep,
      "Set this to specify the separator for values in a testset. "
      "The default value is \",\".");

  bool parallel = false;
  command_cagen->add_flag("--parallel", parallel,
                          "Set this flag to enable parallelization of the "
                          "algorithm execution.");

  bool rand_star = true;
  command_cagen->add_option(
      "--randstar", rand_star,
      "Set this to \"on\" to randomize don't care values and to \"off\" to "
      "not randomize don't care values, i.e. to keep them in the generated "
      "testset. The default value is \"on\".");

  std::string model_file_path{""};
  command_cagen
      ->add_option(
          "model_file", model_file_path,
          "This is the path to the model in ACTS format (TXT version).")
      ->required();

  std::string test_set_file_path{""};
  command_cagen
      ->add_option("testset_file", test_set_file_path,
                   "This is the path where the testset shall be written to.")
      ->required();

  CLI::App *command_cov_measure = app.add_subcommand(
      "covm", "This command measures the coverage of a given testset.");

  command_cov_measure
      ->add_option(
          "--doi", interaction_strength,
          "Specifies the degree of interactions to be measured. Use -1 "
          "for mixed strength as specified in the input file. The "
          "default value is 2.")
      ->check(InteractionStrengthValidator());

  command_cov_measure->add_flag(
      "--progress", show_progress,
      "Set this flag to enable displaying progress information.");

  command_cov_measure->add_option(
      "--sep", sep,
      "Set this to specify the separator for values in a testset. "
      "The default value is \",\".");

  command_cov_measure->add_flag(
      "--parallel", parallel,
      "Set this flag to enable parallelization of the "
      "algorithm execution.");

  command_cov_measure
      ->add_option(
          "model_file", model_file_path,
          "This is the path to the model in ACTS format (TXT version).")
      ->required();

  command_cov_measure
      ->add_option("testset_file", test_set_file_path,
                   "This is the path to the testset.")
      ->required();

  std::string coverage_measurement_file_path{""};
  command_cov_measure
      ->add_option("coverage_measurement_file", coverage_measurement_file_path,
                   "This is the path where the coverage measurement shall be "
                   "written to.")
      ->required();

  app.require_subcommand(0, 1);

  CLI11_PARSE(app, argc, argv);

  if (app.get_subcommands().empty()) {
    if (show_version) {
      std::cout << "citcpp version " << CITCPP_VERSION_MAJOR << "."
                << CITCPP_VERSION_MINOR << "." << CITCPP_VERSION_PATCH
                << std::endl;
    } else {
      std::cout << app.help() << std::flush;
    }

    return 0;
  }

  // Install a signal handler allowing to gracefully abort the computation using
  // Ctrl+C.
  std::signal(SIGINT, signal_handler);

  if (command_cagen->parsed()) {
    return execute_cagen(model_file_path, test_set_file_path,
                         interaction_strength, show_progress, sep, parallel,
                         rand_star);
  }
  if (command_cov_measure->parsed()) {
    return execute_covm(model_file_path, test_set_file_path,
                        coverage_measurement_file_path, interaction_strength,
                        show_progress, sep, parallel);
  }
}
