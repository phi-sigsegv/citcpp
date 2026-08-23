#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <cmath>

// Struct to represent a literal (variable index + sign)
struct Literal {
    int var_idx; // 1-based variable index
    bool is_negative;
};

// Struct to represent a CNF clause
struct Clause {
    std::vector<Literal> literals;
};

// Extracts base name of a file path (without directory and extension)
std::string get_base_name(const std::string& path) {
    size_t last_slash = path.find_last_of("/\\");
    std::string filename = (last_slash == std::string::npos) ? path : path.substr(last_slash + 1);
    size_t last_dot = filename.find_last_of('.');
    if (last_dot != std::string::npos) {
        return filename.substr(0, last_dot);
    }
    return filename;
}

// Parses DIMACS CNF file
void parse_dimacs(const std::string& filepath, int& num_vars, int& num_clauses, std::vector<Clause>& clauses) {
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        throw std::runtime_error("Could not open DIMACS CNF file: " + filepath);
    }

    std::string line;
    num_vars = 0;
    num_clauses = 0;
    clauses.clear();

    bool header_found = false;
    Clause current_clause;

    while (std::getline(infile, line)) {
        // Trim leading whitespace
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            continue; // Empty line
        }
        std::string trimmed = line.substr(first);

        // Check if comment
        if (trimmed[0] == 'c') {
            continue;
        }

        // Check if header/problem line
        if (trimmed[0] == 'p') {
            std::stringstream ss(trimmed);
            std::string p_tok, cnf_tok;
            int h_vars = 0;
            int h_clauses = 0;
            if (ss >> p_tok >> cnf_tok >> h_vars >> h_clauses) {
                if (p_tok == "p" && cnf_tok == "cnf") {
                    num_vars = h_vars;
                    num_clauses = h_clauses;
                    header_found = true;
                    continue;
                }
            }
            throw std::runtime_error("Invalid DIMACS problem line: " + trimmed);
        }

        // Parse clause literals
        std::stringstream ss(trimmed);
        int val;
        while (ss >> val) {
            if (val == 0) {
                clauses.push_back(current_clause);
                current_clause.literals.clear();
            } else {
                int var = std::abs(val);
                bool is_neg = (val < 0);
                current_clause.literals.push_back({var, is_neg});
            }
        }
    }

    // Handle last clause if not terminated by 0
    if (!current_clause.literals.empty()) {
        std::cerr << "Warning: Last clause was not terminated by 0. Adding it anyway.\n";
        clauses.push_back(current_clause);
    }

    // Fallback if header was missing
    if (!header_found) {
        int max_var = 0;
        for (const auto& clause : clauses) {
            for (const auto& lit : clause.literals) {
                if (lit.var_idx > max_var) {
                    max_var = lit.var_idx;
                }
            }
        }
        num_vars = max_var;
        num_clauses = static_cast<int>(clauses.size());
        std::cerr << "Warning: No DIMACS header ('p cnf ...') found. Inferred " 
                  << num_vars << " variables and " << num_clauses << " clauses.\n";
    } else {
        // Validate variable indices in clauses against header
        for (const auto& clause : clauses) {
            for (const auto& lit : clause.literals) {
                if (lit.var_idx > num_vars) {
                    std::cerr << "Warning: Variable index " << lit.var_idx 
                              << " exceeds the variable count specified in header (" << num_vars << "). Resizing variable count.\n";
                    num_vars = lit.var_idx;
                }
            }
        }
    }
}

// Formats a clause into ACTS syntax (e.g. "(p1 = 1) || (p2 = 0)")
std::string format_clause(const Clause& clause) {
    std::string result = "";
    for (size_t i = 0; i < clause.literals.size(); ++i) {
        if (i > 0) {
            result += " || ";
        }
        const auto& lit = clause.literals[i];
        std::string param_name = "p" + std::to_string(lit.var_idx);
        std::string val_str = lit.is_negative ? "0" : "1";
        result += "(" + param_name + " = " + val_str + ")";
    }
    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <cnf_file> <output_file>" << std::endl;
        return 1;
    }

    std::string cnf_path = argv[1];
    std::string output_path = argv[2];

    try {
        int num_vars = 0;
        int num_clauses = 0;
        std::vector<Clause> clauses;

        // 1. Parse DIMACS CNF file
        parse_dimacs(cnf_path, num_vars, num_clauses, clauses);

        if (num_vars <= 0) {
            throw std::runtime_error("Number of variables must be greater than 0.");
        }

        // 2. Write ACTS output file
        std::ofstream outfile(output_path);
        if (!outfile.is_open()) {
            throw std::runtime_error("Could not open output file for writing: " + output_path);
        }

        std::string system_name = get_base_name(cnf_path);
        if (system_name.empty()) {
            system_name = "DIMACS_Converted";
        }

        // [System] section
        outfile << "[System]\n";
        outfile << "Name: " << system_name << "\n\n";

        // [Parameter] section
        outfile << "[Parameter]\n";
        for (int i = 1; i <= num_vars; ++i) {
            outfile << "p" << i << " (int) : 0, 1\n";
        }
        outfile << "\n";

        // [Constraint] section
        outfile << "[Constraint]\n";
        for (const auto& clause : clauses) {
            if (clause.literals.empty()) {
                std::cerr << "Warning: Skipping empty clause in output.\n";
                continue; // skip empty clauses
            }
            outfile << format_clause(clause) << "\n";
        }

        std::cout << "Successfully converted DIMACS CNF to ACTS format!\n";
        std::cout << "  System name: " << system_name << "\n";
        std::cout << "  Variables:   " << num_vars << "\n";
        std::cout << "  Constraints: " << clauses.size() << " clauses\n";
        std::cout << "  Output file: " << output_path << std::endl;

    } catch (const std::exception& ex) {
        std::cerr << "Error during conversion: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
