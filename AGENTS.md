# citcpp

`citcpp` is a high-performance tool for Combinatorial Interaction Testing (CIT) implemented in C++20. It provides capabilities for generating covering arrays and measuring t-way interaction coverage of existing test sets.

## Project Overview

- **Goal:** Provide a fast and scalable CIT tool supporting constraints and mixed-strength interactions.
- **Core Algorithm:** Implements the IPOG (In-Parameter-Order General) algorithm for covering array generation.
- **Constraint Handling:** Uses multi-core decision diagrams (LDDs via the [Sylvan](https://github.com/trolando/sylvan) library) for efficient constraint evaluation.
- **Main Technologies:**
    - **Language:** C++20
    - **Build System:** CMake
    - **Decision Diagrams:** Sylvan & Lace (work-stealing framework)
    - **CLI:** CLI11
    - **Parsing:** cpp-peglib (for ACTS model files)
    - **Testing:** doctest

## Directory Structure

- `citcpp_lib/`: The core library containing the CIT logic.
    - `include/citcpp/`: Public API headers.
    - `src/`: Implementation files.
    - `src/detail/`: Internal implementation details, including IPOG and decision diagram integration.
- `src/`: Source code for the `citcpp` CLI application.
    - `acts_model_parser.cpp`: Parser for ACTS (txt) model format.
    - `main.cpp`: Application entry point and CLI definition.
- `3rd_party/`: Bundled external dependencies.
- `tests/`: Comprehensive unit tests using `doctest`.
- `plans/`: Design documents and optimization plans (e.g., `ipog_vertical_extension_optimization.md`).

## Building and Running

### Prerequisites

- A C++20 compliant compiler (e.g., GCC 10+, Clang 10+, MSVC 2019+).
- CMake 3.16 or higher.

### Build Instructions

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Running the CLI

The `citcpp` executable supports two main subcommands: `cagen` and `covm`.

#### Covering Array Generation (`cagen`)

Generates a covering array for a given model.

```bash
./citcpp cagen <model_file> <output_testset_file> [options]
```

**Common Options:**
- `--doi <int>`: Degree of interaction (default: 2). Use `-1` for mixed strength.
- `--num-threads <int>`: Number of threads to use (0 for auto).
- `--progress`: Show computation progress.
- `--seed-testset_file <file>`: Extend an existing test set.
- `--randstar <on|off>`: Randomize "don't care" values (default: on).

#### Coverage Measurement (`covm`)

Measures the t-way coverage of an existing test set.

```bash
./citcpp covm <model_file> <testset_file> <output_json_file> [options]
```

## Development Conventions

- **C++ Standard:** C++20 is strictly required.
- **Coding Style:** The project uses `.clang-format` for consistent formatting.
- **Testing:** New features should include unit tests in the `tests/` directory. Tests are discovered automatically via `doctest_discover_tests`.
- **Parallelism:** The project utilizes the Lace work-stealing framework for parallel execution. Ensure thread-safety when modifying core algorithms.
- **Constraint Handling:** Constraints are handled via Sylvan LDDs. Modification to constraint logic should be done carefully within `citcpp_lib/src/detail/constraint_handler_sylvan_ldd.cpp`.

## Key Files

- `citcpp_lib/include/citcpp/citcpp.hpp`: Main entry point for the library API.
- `citcpp_lib/src/detail/citcpp_ipog.cpp`: Implementation of the IPOG algorithm.
- `src/acts_model_parser.cpp`: Logic for parsing ACTS model files.
- `src/main.cpp`: CLI argument parsing and execution flow.
