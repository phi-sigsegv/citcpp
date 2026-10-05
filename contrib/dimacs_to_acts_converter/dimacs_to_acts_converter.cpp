#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <cmath>
#include <map>
#include <regex>
#include <unordered_set>

// Struct to represent a literal (variable index + sign)
struct Literal {
    int var_idx; // 1-based variable index
    bool is_negative;
};

// Struct to represent a CNF clause
struct Clause {
    std::vector<Literal> literals;
};

// Information about a variable taken from a "c <id>[$] <name> ..." comment
struct VarComment {
    std::string name;
    bool has_dollar;
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

// Parses a variable naming comment such as "c 12 FOO", "c 13$ _X2" or
// "c 14 FOO bool". Returns false if the comment does not follow this pattern.
bool parse_var_comment(const std::string& comment, int& var_idx, VarComment& info) {
    static const std::regex pattern(R"(^c\s+([0-9]+)(\$?)\s+(\S+).*$)");
    std::smatch match;
    if (!std::regex_match(comment, match, pattern)) {
        return false;
    }
    var_idx = std::stoi(match[1].str());
    info.has_dollar = match[2].length() > 0;
    info.name = match[3].str();
    return var_idx > 0;
}

// Parses DIMACS CNF file
void parse_dimacs(const std::string& filepath, int& num_vars, int& num_clauses, std::vector<Clause>& clauses,
                  std::map<int, VarComment>& var_comments) {
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        throw std::runtime_error("Could not open DIMACS CNF file: " + filepath);
    }

    std::string line;
    num_vars = 0;
    num_clauses = 0;
    clauses.clear();
    var_comments.clear();

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
            int var_idx = 0;
            VarComment info;
            if (parse_var_comment(trimmed, var_idx, info)) {
                var_comments[var_idx] = info;
            }
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

// Checks whether a name is a valid identifier in the ACTS format
bool is_acts_identifier(const std::string& name) {
    static const std::regex pattern(R"(^[a-zA-Z_][a-zA-Z0-9_]*$)");
    return std::regex_match(name, pattern);
}

// Checks whether a variable is an auxiliary variable introduced by the CNF
// transformation (Tseitin variables of kconfigreader and LVAT, e.g. "c 3146$ _X2",
// "c 11254$ _X4055_m" or "c 5703$ x2").
bool is_auxiliary_variable(const VarComment& info) {
    static const std::regex pattern(R"(^(_X|x)[0-9]+(_m)?$)");
    return info.has_dollar && std::regex_match(info.name, pattern);
}

// Determines the ACTS parameter name for every variable (index 0 is unused).
// The name from the variable comment is used if it is a valid ACTS identifier,
// which is not used by another variable. Otherwise the name is "p<i>".
std::vector<std::string> create_param_names(int num_vars, const std::map<int, VarComment>& var_comments) {
    std::vector<std::string> names(num_vars + 1);
    std::unordered_set<std::string> used_names;
    for (int i = 1; i <= num_vars; ++i) {
        auto it = var_comments.find(i);
        if (it != var_comments.end() && is_acts_identifier(it->second.name)) {
            if (used_names.insert(it->second.name).second) {
                names[i] = it->second.name;
            }
        }
    }
    for (int i = 1; i <= num_vars; ++i) {
        if (!names[i].empty()) {
            continue;
        }
        std::string name = "p" + std::to_string(i);
        while (!used_names.insert(name).second) {
            name = "_" + name;
        }
        names[i] = name;
    }
    return names;
}

// Formats a clause into ACTS syntax (e.g. "(p1 = 1) || (p2 = 0)")
std::string format_clause(const Clause& clause, const std::vector<std::string>& param_names) {
    std::string result = "";
    for (size_t i = 0; i < clause.literals.size(); ++i) {
        if (i > 0) {
            result += " || ";
        }
        const auto& lit = clause.literals[i];
        const std::string& param_name = param_names[lit.var_idx];
        std::string val_str = lit.is_negative ? "0" : "1";
        result += "(" + param_name + " = " + val_str + ")";
    }
    return result;
}

void print_usage(const char* program) {
    std::cerr << "Usage: " << program << " [--strength <t>] [--keep-aux] <cnf_file> <output_file>\n"
              << "  --strength <t>  Write a [Relation] section of strength t containing all\n"
              << "                  variables except auxiliary ones (use 'citcpp cagen --doi -1').\n"
              << "  --keep-aux      Do not detect auxiliary variables; the relation contains all\n"
              << "                  variables." << std::endl;
}

int main(int argc, char* argv[]) {
    int strength = 0;
    bool keep_aux = false;
    std::vector<std::string> positional;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--strength") {
            if (i + 1 >= argc) {
                print_usage(argv[0]);
                return 1;
            }
            try {
                strength = std::stoi(argv[++i]);
            } catch (const std::exception&) {
                strength = 0;
            }
            if (strength < 1) {
                std::cerr << "Error: The strength must be a positive integer." << std::endl;
                return 1;
            }
        } else if (arg == "--keep-aux") {
            keep_aux = true;
        } else {
            positional.push_back(arg);
        }
    }
    if (positional.size() != 2) {
        print_usage(argv[0]);
        return 1;
    }

    std::string cnf_path = positional[0];
    std::string output_path = positional[1];

    try {
        int num_vars = 0;
        int num_clauses = 0;
        std::vector<Clause> clauses;
        std::map<int, VarComment> var_comments;

        // 1. Parse DIMACS CNF file
        parse_dimacs(cnf_path, num_vars, num_clauses, clauses, var_comments);

        if (num_vars <= 0) {
            throw std::runtime_error("Number of variables must be greater than 0.");
        }

        const std::vector<std::string> param_names = create_param_names(num_vars, var_comments);

        // Variables to be covered by the relation (all non-auxiliary variables)
        std::vector<int> relation_vars;
        for (int i = 1; i <= num_vars; ++i) {
            auto it = var_comments.find(i);
            if (keep_aux || it == var_comments.end() || !is_auxiliary_variable(it->second)) {
                relation_vars.push_back(i);
            }
        }
        if (strength > 0 && relation_vars.empty()) {
            throw std::runtime_error("All variables are auxiliary variables; cannot create a relation.");
        }
        if (strength > static_cast<int>(relation_vars.size())) {
            std::cerr << "Warning: Strength " << strength << " exceeds the number of non-auxiliary variables ("
                      << relation_vars.size() << "). Reducing the strength.\n";
            strength = static_cast<int>(relation_vars.size());
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
            outfile << param_names[i] << " (int) : 0, 1\n";
        }
        outfile << "\n";

        // [Relation] section
        if (strength > 0) {
            outfile << "[Relation]\n";
            outfile << "R1 : (";
            for (size_t i = 0; i < relation_vars.size(); ++i) {
                if (i > 0) {
                    outfile << ", ";
                }
                outfile << param_names[relation_vars[i]];
            }
            outfile << ", " << strength << ")\n\n";
        }

        // [Constraint] section
        outfile << "[Constraint]\n";
        for (const auto& clause : clauses) {
            if (clause.literals.empty()) {
                std::cerr << "Warning: Skipping empty clause in output.\n";
                continue; // skip empty clauses
            }
            outfile << format_clause(clause, param_names) << "\n";
        }

        std::cout << "Successfully converted DIMACS CNF to ACTS format!\n";
        std::cout << "  System name: " << system_name << "\n";
        std::cout << "  Variables:   " << num_vars << "\n";
        std::cout << "  Named:       " << var_comments.size() << " variables with name comments\n";
        if (strength > 0) {
            std::cout << "  Relation:    " << relation_vars.size() << " variables, strength " << strength << " ("
                      << (num_vars - static_cast<int>(relation_vars.size())) << " auxiliary variables excluded)\n";
        }
        std::cout << "  Constraints: " << clauses.size() << " clauses\n";
        std::cout << "  Output file: " << output_path << std::endl;

    } catch (const std::exception& ex) {
        std::cerr << "Error during conversion: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
