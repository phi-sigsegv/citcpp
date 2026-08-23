#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <stdexcept>

// Struct to map a flat variable index to a parameter and its value
struct VarMap {
    int param_idx;
    int value_idx;
};

// Struct to represent a literal (variable index + sign)
struct Literal {
    int var_idx;
    bool is_negative;
};

// Struct to represent a CNF clause
struct Clause {
    std::vector<Literal> literals;
};

// Helper to strip comments (standard comment prefixes: #, //, c)
std::string strip_comments(const std::string& line) {
    std::string clean = "";
    for (size_t i = 0; i < line.length(); ++i) {
        if (line[i] == '#' || (line[i] == '/' && i + 1 < line.length() && line[i+1] == '/')) {
            break;
        }
        // Match DIMACS/CASA style comments starting with 'c' followed by space or end of line
        if (i == 0 && line[i] == 'c' && (line.length() == 1 || std::isspace(static_cast<unsigned char>(line[1])))) {
            break;
        }
        clean += line[i];
    }
    return clean;
}

// Read the next non-empty, non-comment line
bool get_next_valid_line(std::ifstream& infile, std::string& line) {
    while (std::getline(infile, line)) {
        line = strip_comments(line);
        // Trim leading and trailing whitespace
        auto is_space = [](unsigned char ch) { return std::isspace(ch); };
        line.erase(line.begin(), std::find_if_not(line.begin(), line.end(), is_space));
        line.erase(std::find_if_not(line.rbegin(), line.rend(), is_space).base(), line.end());
        if (!line.empty()) {
            return true;
        }
    }
    return false;
}

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

// Parses integers from the model file while ignoring comments
std::vector<int> read_model_integers(const std::string& filepath) {
    std::vector<int> ints;
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        throw std::runtime_error("Could not open model file: " + filepath);
    }
    std::string line;
    while (std::getline(infile, line)) {
        std::string clean = strip_comments(line);
        std::stringstream ss(clean);
        std::string token;
        while (ss >> token) {
            try {
                ints.push_back(std::stoi(token));
            } catch (...) {
                // Skip non-integer tokens
            }
        }
    }
    return ints;
}

// Parses constraints from the constraints file
std::vector<Clause> read_casa_constraints(const std::string& filepath) {
    std::vector<Clause> clauses;
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        std::cerr << "Warning: Constraints file " << filepath << " could not be opened. Assuming 0 constraints." << std::endl;
        return clauses;
    }

    std::string line;
    if (!get_next_valid_line(infile, line)) {
        return clauses;
    }

    int num_clauses = 0;
    try {
        num_clauses = std::stoi(line);
    } catch (...) {
        throw std::runtime_error("Invalid constraints file header (expected number of clauses): " + line);
    }

    for (int c = 0; c < num_clauses; ++c) {
        if (!get_next_valid_line(infile, line)) {
            throw std::runtime_error("Unexpected end of constraints file while reading clause literal count at index " + std::to_string(c));
        }
        int num_literals = 0;
        try {
            num_literals = std::stoi(line);
        } catch (...) {
            throw std::runtime_error("Invalid clause literal count: " + line);
        }

        if (!get_next_valid_line(infile, line)) {
            throw std::runtime_error("Unexpected end of constraints file while reading literals for clause at index " + std::to_string(c));
        }

        std::stringstream ss(line);
        std::string token;
        std::vector<Literal> literals;
        bool pending_negative = false;

        while (ss >> token) {
            if (token == "-") {
                pending_negative = true;
            } else if (token == "+") {
                pending_negative = false;
            } else if (token[0] == '-') {
                try {
                    int var_idx = std::stoi(token.substr(1));
                    literals.push_back({var_idx, true});
                } catch (...) {
                    throw std::runtime_error("Invalid literal token: " + token);
                }
                pending_negative = false;
            } else if (token[0] == '+') {
                try {
                    int var_idx = std::stoi(token.substr(1));
                    literals.push_back({var_idx, false});
                } catch (...) {
                    throw std::runtime_error("Invalid literal token: " + token);
                }
                pending_negative = false;
            } else {
                try {
                    int var_idx = std::stoi(token);
                    literals.push_back({var_idx, pending_negative});
                } catch (...) {
                    throw std::runtime_error("Invalid literal token: " + token);
                }
                pending_negative = false;
            }
        }

        if (static_cast<int>(literals.size()) != num_literals) {
            std::cerr << "Warning: Clause " << c << " expected " << num_literals 
                      << " literals, but parsed " << literals.size() << ". Proceeding with parsed literals." << std::endl;
        }
        clauses.push_back({literals});
    }

    return clauses;
}

// Formats a literal into ACTS syntax (e.g. "(p3 != 2)")
std::string format_literal(const Literal& lit, const std::vector<VarMap>& var_map) {
    if (lit.var_idx < 0 || lit.var_idx >= static_cast<int>(var_map.size())) {
        throw std::runtime_error("Literal variable index " + std::to_string(lit.var_idx) 
                                 + " is out of bounds (total variables mapping size: " 
                                 + std::to_string(var_map.size()) + ")");
    }
    const VarMap& vm = var_map[lit.var_idx];
    std::string param_name = "p" + std::to_string(vm.param_idx);
    std::string op = lit.is_negative ? "!=" : "=";
    return "(" + param_name + " " + op + " " + std::to_string(vm.value_idx) + ")";
}

// Formats a clause into ACTS syntax (e.g. "(p3 != 2) || (p5 = 0)")
std::string format_clause(const Clause& clause, const std::vector<VarMap>& var_map) {
    std::string result = "";
    for (size_t i = 0; i < clause.literals.size(); ++i) {
        if (i > 0) {
            result += " || ";
        }
        result += format_literal(clause.literals[i], var_map);
    }
    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <model_file> <constraints_file> <output_file>" << std::endl;
        return 1;
    }

    std::string model_path = argv[1];
    std::string constraints_path = argv[2];
    std::string output_path = argv[3];

    try {
        // 1. Parse model file
        std::vector<int> model_ints = read_model_integers(model_path);
        if (model_ints.size() < 2) {
            throw std::runtime_error("CASA model file must contain at least strength and parameter count.");
        }

        int num_parameters = 0;
        int strength = 2;
        std::vector<int> domain_sizes;

        // Automatically detect model format (Format A: strength, params, domains vs Format B: params, strength, domains)
        int opt1_num = model_ints[1]; // candidate num_params
        int opt2_num = model_ints[0]; // candidate num_params
        int remaining = static_cast<int>(model_ints.size()) - 2;

        if (opt1_num == remaining) {
            num_parameters = opt1_num;
            strength = model_ints[0];
        } else if (opt2_num == remaining) {
            num_parameters = opt2_num;
            strength = model_ints[1];
        } else {
            // Fallback: make a heuristic decision based on what fits within the file
            if (opt1_num <= remaining && opt1_num > 0) {
                num_parameters = opt1_num;
                strength = model_ints[0];
            } else if (opt2_num <= remaining && opt2_num > 0) {
                num_parameters = opt2_num;
                strength = model_ints[1];
            } else {
                num_parameters = remaining;
                strength = std::min(model_ints[0], model_ints[1]);
            }
        }

        if (num_parameters <= 0) {
            throw std::runtime_error("Detected number of parameters is non-positive: " + std::to_string(num_parameters));
        }

        for (int i = 0; i < num_parameters; ++i) {
            if (2 + i < static_cast<int>(model_ints.size())) {
                domain_sizes.push_back(model_ints[2 + i]);
            } else {
                domain_sizes.push_back(2); // fallback default domain size
            }
        }

        // 2. Build flat variable map
        std::vector<VarMap> var_map;
        for (int p = 0; p < num_parameters; ++p) {
            for (int v = 0; v < domain_sizes[p]; ++v) {
                var_map.push_back({p, v});
            }
        }

        // 3. Parse constraints file
        std::vector<Clause> clauses = read_casa_constraints(constraints_path);

        // 4. Generate ACTS output file
        std::ofstream outfile(output_path);
        if (!outfile.is_open()) {
            throw std::runtime_error("Could not open output file for writing: " + output_path);
        }

        std::string system_name = get_base_name(model_path);
        if (system_name.empty()) {
            system_name = "CASA_Converted";
        }

        // [System] section
        outfile << "[System]\n";
        outfile << "Name: " << system_name << "\n\n";

        // [Parameter] section
        outfile << "[Parameter]\n";
        for (int p = 0; p < num_parameters; ++p) {
            outfile << "p" << p << " (int) : ";
            for (int v = 0; v < domain_sizes[p]; ++v) {
                if (v > 0) {
                    outfile << ", ";
                }
                outfile << v;
            }
            outfile << "\n";
        }
        outfile << "\n";

        // [Relation] section (expressing the default strength)
        outfile << "[Relation]\n";
        outfile << "R1 : (";
        for (int p = 0; p < num_parameters; ++p) {
            if (p > 0) {
                outfile << ", ";
            }
            outfile << "p" << p;
        }
        outfile << ", " << strength << ")\n\n";

        // [Constraint] section
        outfile << "[Constraint]\n";
        for (const auto& clause : clauses) {
            if (clause.literals.empty()) {
                continue; // skip empty clauses
            }
            outfile << format_clause(clause, var_map) << "\n";
        }

        std::cout << "Successfully converted CASA model and constraints to ACTS format!\n";
        std::cout << "  System name: " << system_name << "\n";
        std::cout << "  Parameters:  " << num_parameters << "\n";
        std::cout << "  Strength:    " << strength << "\n";
        std::cout << "  Constraints: " << clauses.size() << " clauses\n";
        std::cout << "  Output file: " << output_path << std::endl;

    } catch (const std::exception& ex) {
        std::cerr << "Error during conversion: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
