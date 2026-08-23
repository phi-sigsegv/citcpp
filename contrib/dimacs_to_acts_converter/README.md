# DIMACS CNF to ACTS Converter

This program converts Boolean satisfiability formulas in DIMACS CNF format to ACTS (Advanced Combinatorial Testing System) input model format.

## Building the program

To build the converter, run the following commands from the project root:

```bash
mkdir -p build
cd build
cmake ..
make
```

This will produce the executable binary `dimecs_to_acts_converter`.

## Usage

```bash
./dimecs_to_acts_converter <cnf_file> <output_file>
```

### Arguments:
* `<cnf_file>`: Path to the input DIMACS CNF file.
* `<output_file>`: Path to write the output ACTS model file.

## Mapping details
* Each CNF variable $i$ is mapped to a parameter `pi` in ACTS.
* Since CNF variables are Boolean, they are represented as integer parameters in ACTS with the domain `0, 1` (representing false and true, respectively).
* Positive literals $i$ are mapped to `(pi = 1)`.
* Negative literals $-i$ are mapped to `(pi = 0)`.
* Each CNF clause is mapped to a disjunction constraint in ACTS.
