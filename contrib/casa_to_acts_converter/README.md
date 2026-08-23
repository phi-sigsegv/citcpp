# CASA to ACTS Converter

This program converts combinatorial models and constraints in the CASA format into the ACTS (Advanced Combinatorial Testing System) input model format.

## Building the program

To build the converter, run the following commands from this directory:

```bash
mkdir -p build
cd build
cmake ..
make
```

This will produce the executable binary `casa_to_acts_converter`.

## Usage

```bash
./casa_to_acts_converter <model_file> <constraints_file> <output_file>
```

### Arguments:
* `<model_file>`: Path to the CASA model file.
* `<constraints_file>`: Path to the CASA constraints file.
* `<output_file>`: Path to write the output ACTS model file.

## Format Details

### CASA Model File:
A text file where comments start with `#`, `//`, or `c `. The non-comment integers specify:
1. Strength (e.g., 2 for pairwise)
2. Number of parameters $P$
3. $P$ integers representing the domain size of each parameter

Example (`some.model`):
```text
3
15
4 2 2 2 2 2 2 2 2 2 2 2 2 2 2
```

### CASA Constraints File:
Specifies forbidden combinations of flat variable values in CNF format:
1. Total number of constraint clauses.
2. For each clause:
   * The number of literals in the clause.
   * A space-separated list of literals (prefixed with `-` if negated, or `+` if positive), mapping to flat variable indices.

Example (`some.constraints`):
```text
3
2
- 2 - 20
2
- 20 - 1
2
- 20 - 3
```
*(Flat variable indices are mapped sequentially based on parameter domain sizes. For example, if `p0` has domain size 4, variables 0, 1, 2, 3 correspond to `p0 = 0`, `p0 = 1`, `p0 = 2`, `p0 = 3` respectively)*
