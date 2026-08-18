# citcpp

`citcpp` is a high-performance tool and C++ library for **Combinatorial Interaction Testing (CIT)** implemented in modern C++20. It provides state-of-the-art capabilities for generating covering arrays (subcommand `cagen`) and measuring the t-way interaction coverage of existing test sets (subcommand `covm`).

## Key Features

- **High Performance & Scalability**: Built from the ground up in C++20, leveraging parallel computing techniques.
- **Advanced Constraint Handling**: Integrates multi-core Interval Decision Diagrams (IDDs) encoded in terms of List Decision Diagrams (LDDs) via the [Sylvan](https://github.com/trolando/sylvan) library to efficiently represent and evaluate complex constraints on parameter spaces.
- **Parallel Execution**: Uses the [Lace](https://github.com/trolando/lace) work-stealing framework to distribute processing load across multiple CPU cores.
- **IPOG Algorithm**: Implements the In-Parameter-Order General (IPOG) algorithm for covering array generation.
- **Mixed Strength Support**: Allows specifying custom parameter groups (relations) with different interaction strengths.
- **Seed Extension**: Can extend an existing, partially-defined test set to satisfy the target interaction coverage.
- **Flexible CLI & Programmatic C++ API**: Easily used as a standalone command-line utility or integrated into other C++ applications as a shared/static library.

---

## Building and Installation

### Prerequisites

To compile `citcpp`, you will need:
- A C++20 compliant compiler (e.g., GCC 10+, Clang 10+).
- CMake 3.23 or higher.
- A build tool (e.g., Make, Ninja).

### Compilation Steps

1. Clone the repository:
   ```bash
   git clone <repository_url>
   cd citcpp
   ```
2. Create and enter a build directory:
   ```bash
   mkdir build && cd build
   ```
3. Configure the build (set build type to `Release` for optimal performance):
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```
4. Build the executable and library:
   ```bash
   cmake --build .
   ```

### Running Tests

Unit tests are written using the `doctest` framework and are managed via CMake's CTest integration. To run all tests, execute:
```bash
ctest --output-on-failure
```
Alternatively, you can run the individual test binaries directly from the `tests/` directory inside your build folder (e.g., `./tests/test_cagen`).

---

## CLI Usage

The built executable `citcpp` provides two primary commands: `cagen` and `covm`.

```bash
./citcpp [options] [subcommand]
```

**Global Options:**
* `-v, --version`: Print the version information and exit.
* `-h, --help`: Show the global help message.

---

### 1. Covering Array Generation (`cagen`)

Generates a covering array (test set) for a given input model.

#### Command Syntax
```bash
./citcpp cagen <model_file> <testset_file> [options]
```

#### Positional Arguments
* `model-file` (Required): The path to the input model file in ACTS format (`.txt`).
* `testset-file` (Required): The path where the generated test set (covering array) should be saved.

#### Command-Specific Options
* `--doi <int>`: Degree of Interaction (interaction strength). Specifies the degree of parameter combinations to cover (default: `2`). Set to `-1` for mixed-strength as specified in the model file's `[Relation]` section.
* `--num-threads <int>`: Number of threads to use for execution (default: `1`). Specifying `0` selects the thread count automatically based on the hardware capability.
* `--mem-limit <int>`: Sets the memory limit used for the constraint handler in gigabytes (default: `8` GB).
* `--progress`: Show real-time computation progress.
* `--seed-testset-file <file>`: The path to an existing test set file to be used as a seed and extended to full coverage.
* `--randstar <on|off>`: Control "don't care" (wildcard) values randomization. Set to `on` (default) to replace don't care values with randomized valid values, or `off` to preserve them as wildcard characters (`*`) in the output.
* `--sep <string>`: Specify the delimiter used to separate parameter values in both input seed files and output test set files (default: `, `).

#### Example Command
```bash
./citcpp cagen model.txt output.csv --doi 3 --progress
```

---

### 2. Coverage Measurement (`covm`)

Measures the t-way interaction coverage of an existing test set against a given model.

#### Command Syntax
```bash
./citcpp covm <model_file> <testset_file> <coverage_measurement_file> [options]
```

#### Positional Arguments
* `model-file` (Required): The path to the input model file in ACTS format.
* `testset-file` (Required): The path to the existing test set file whose coverage is being measured.
* `coverage-measurement-file` (Required): The path where the final measurement results will be saved in JSON format.

#### Command-Specific Options
* `--doi <int>`: Specifies the degree of interactions to measure (default: `2`). Set to `-1` for mixed-strength coverage defined in the model.
* `--num-threads <int>`: Number of threads to use for execution (default: `1`). Specifying `0` selects the thread count automatically based on the hardware capability.
* `--mem-limit <int>`: Sets the memory limit used for the constraint handler in gigabytes (default: `8` GB).
* `--progress`: Show real-time computation progress.
* `--sep <string>`: Value separator used in the test set file (default: `, `).

#### Example Command
```bash
./citcpp covm model.txt testset.csv report.json --doi 2 --progress
```

---

## File Formats

### 1. Input Model File (ACTS Text Format)

The input model must be defined in the standard ACTS text format. It contains four main sections:

- **`[System]`**: Defines the system name.
- **`[Parameter]`**: Specifies parameters, types (`boolean`, `int`, or `enum`), and their domain values.
- **`[Relation]`**: (Optional) For mixed-strength interactions. Defines specific groups of parameters and their target strength.
- **`[Constraint]`**: (Optional) Lists boolean constraint propositions. All generated tests are guaranteed to satisfy all constraints.

#### Example Model (`model.txt`)
```text
[System]
Name: MyWebSystem

[Parameter]
OS (enum) : Linux, Windows, macOS
Browser (enum) : Chrome, Firefox, Safari
HTTPS (boolean) : true, false
Connections (int) : 10, 100, 1000

[Relation]
R1 : (OS, Browser, 2)

[Constraint]
(Browser == "Safari") => (OS == "macOS")
(Connections == 1000) => (HTTPS == true)
```

#### Differences to ACTS regarding supported features

`citcpp` does not support marking values as invalid (which helps for negative testing) and also has no support for base-choice values. The constraint language supported by `citcpp` is a subset of the one supported by ACTS. The following grammar defines it:

```
<Constraint> ::= <Simple_Constraint> | <Constraint> <Boolean_Op> <Constraint>
<Simple_Constraint> ::= <Parameter> <Relational_Op> <Value>
<Boolean_Op> ::= "&&" | "||" | "=>"
<Relational_Op> ::= "=" | "!=" | ">" | "<" | ">=" | "<="
```

So what is missing compared to ACTS is support for arithmetic expressions and relational operators comparing different parameters.

### 2. Output Test Set Format (CSV)

The output is written as a comma-separated (or custom separator) text file. The first row contains the parameter names, followed by the test rows.

```csv
OS,Browser,HTTPS,Connections
Linux,Chrome,false,10
macOS,Safari,true,100
Windows,Firefox,true,1000
...
```

### 3. Coverage Measurement JSON Report

The `covm` subcommand produces a highly detailed JSON report describing the coverage achieved by the test set:

```json
{
  "invalid_tests": [],
  "R1": {
    "num_tuples_to_cover": 9,
    "tuples_covered_by_tests": [
      3,
      5,
      8,
      9
    ],
    "num_param_combinations_to_cover": 1,
    "param_combinations_coverage": [
      {
        "coverage": 0.0,
        "num_param_combos": 1
      },
      ...
      {
        "coverage": 100.0,
        "num_param_combos": 1
      }
    ]
  }
}
```
- `invalid_tests`: A list of indices corresponding to test rows in the input CSV that violate model constraints (and were therefore excluded from measurement).
- `num_tuples_to_cover`: The total number of valid parameter value tuples (combinations) that exist in the system.
- `tuples_covered_by_tests`: A cumulative array showing how many unique tuples have been covered up to that specific test row. For instance, index `k` contains the number of unique combinations covered by the first `k + 1` tests.
- `param_combinations_coverage`: The distribution of coverage levels (incremented in steps of 5%) showing the number of parameter combinations meeting or exceeding that coverage fraction.

---

## C++ Library API

`citcpp` can be integrated directly into C++ applications by linking against the `citcpp_lib` target. The public header is `<citcpp/citcpp.hpp>`.

### Programmatically Creating a Model

The following example demonstrates how to build a model, add parameters, configure and define implications, and generate a covering array:

```cpp
#include <iostream>
#include <memory>
#include <future>
#include <citcpp/citcpp.hpp>

int main() {
    using namespace citcpp;

    // 1. Construct the model
    model web_model;
    web_model.set_name("WebConfiguration");

    // Add Enum Parameter
    web_model.add_parameter(parameter()
        .type(parameter_type::ENUM)
        .name("OS")
        .values({{"Linux"}, {"Windows"}, {"macOS"}}));

    // Add Boolean Parameter
    web_model.add_parameter(parameter()
        .type(parameter_type::BOOLEAN)
        .name("HTTPS")
        .values({{true}, {false}}));

    // Add Integer Parameter
    web_model.add_parameter(parameter()
        .type(parameter_type::INTEGER)
        .name("Connections")
        .values({{10}, {100}}));

    // 2. Add Constraints: (OS == "macOS") => (HTTPS == true)
    auto os_is_macos = std::make_shared<enum_proposition>(
        parameter_reference("OS"), relational_operator::EQ, "macOS"
    );
    auto https_is_true = std::make_shared<boolean_proposition>(
        parameter_reference("HTTPS"), relational_operator::EQ, true
    );
    web_model.add_constraint(
        std::make_shared<implication>(std::move(os_is_macos), std::move(https_is_true))
    );

    // 3. Configure the covering array generation
    covering_array_computation_config config;
    config.with_number_of_threads(0) // 0 for auto-detection
          .with_replace_dont_care_values(true)
          .with_value_separator(",");

    // 4. Trigger computation (2-way coverage)
    std::unique_ptr<cagen_exec_handle_ipog> handle =
        compute_covering_array_ipog(web_model, 2, config);

    // 5. Retrieve the results asynchronously
    std::future<cagen_exec_result> future_result = handle->get_test_set();
    cagen_exec_result exec_result = future_result.get();

    if (exec_result.get_result_code() == cagen_exec_result::cagen_result_code::COVERING_ARRAY_GENERATION_COMPLETED) {
        const test_set& generated_tests = exec_result.get_result();
        std::cout << "Successfully generated " << generated_tests.get_list_of_tests().size() << " tests!\n";
        std::cout << generated_tests << std::endl;
    } else {
        std::cerr << "Generation failed: " << exec_result.get_error_message() << std::endl;
    }

    return 0;
}
```

### Measuring Coverage Programmatically

Similarly, to measure the coverage of an existing test set programmatically:

```cpp
#include <iostream>
#include <memory>
#include <future>
#include <citcpp/citcpp.hpp>

void measure_test_set_coverage(const citcpp::model& model, const citcpp::test_set& tests) {
    using namespace citcpp;

    coverage_measurement_config config;
    config.with_number_of_threads(2)
          .with_value_separator(",");

    std::unique_ptr<covm_exec_handle> handle = measure_coverage(model, tests, 2, config);
    std::future<covm_exec_result> future_result = handle->get_coverage_measurement();
    covm_exec_result exec_result = future_result.get();

    if (exec_result.get_result_code() == covm_exec_result::covm_result_code::COVERAGE_MEASUREMENT_COMPLETED) {
        // The default relation ID for standard doi-way interaction is an empty string ("")
        const coverage_measurement& measurement = exec_result.get_result().at("");
        std::cout << "Combinations covered: " 
                  << measurement.get_covered_tuples().back() << " / " 
                  << measurement.get_number_of_combinations_to_cover() << "\n";
    }
}
```

### Parsing Model and Test Set Files

The library includes built-in parsers to load models from ACTS files and test sets from CSV files:

```cpp
#include <citcpp/acts_model_parser.hpp>
#include <citcpp/test_set_parser.hpp>
#include <string_view>
#include <iostream>

void parse_files(std::string_view acts_file_content, std::string_view csv_file_content) {
    using namespace citcpp;

    // Parse model
    acts_model_parser model_parser;
    model web_model;
    if (!model_parser.parse_input_model(acts_file_content, web_model)) {
        std::cerr << "Model Parsing Error: " << model_parser.get_last_error_message() << std::endl;
        return;
    }

    // Parse test set based on model
    test_set_parser testset_parser(web_model, ",");
    test_set existing_tests;
    if (!testset_parser.parse_test_set(csv_file_content, existing_tests)) {
        std::cerr << "Test Set Parsing Error: " << testset_parser.get_last_error_message() << std::endl;
        return;
    }

    std::cout << "Successfully parsed model: " << web_model.get_name() << "\n";
    std::cout << "Successfully parsed " << existing_tests.get_list_of_tests().size() << " tests.\n";
}
```

---

## Technical Details & Architecture

`citcpp` utilizes a modular and highly optimized architecture designed for maximum performance:
- **Decision Diagrams for Constraint Handling**: Evaluating constraints in large combinatorial spaces is typically a major bottleneck in CIT. `citcpp` solves this by building Interval Decision Diagrams (IDDs) encoded in terms of List Decision Diagrams (LDDs) using [Sylvan](https://github.com/trolando/sylvan) to represent valid parameter assignments as paths.
- **Asynchronous Execution Model**: Computation handles (`cagen_exec_handle` and `covm_exec_handle`) return immediately upon initiation and work in a separate thread. Clients can monitor execution phase, initialize progress, processed combination ratios, and even trigger graceful abort requests safely across threads.
