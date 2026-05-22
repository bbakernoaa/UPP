#include <fkYAML/node.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include "yaml_reader.h"

static fkyaml::node root_node;

extern "C" {

int yaml_load_file(const char* filename) {
    try {
        std::ifstream ifs(filename);
        if (!ifs) return -1;
        root_node = fkyaml::node::deserialize(ifs);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "YAML Load Error: " << e.what() << std::endl;
        return -2;
    }
}

void yaml_free() {
    root_node = fkyaml::node();
}

int yaml_get_paramset_count() {
    try {
        if (root_node.contains("paramset") && root_node["paramset"].is_sequence()) {
            return (int)root_node["paramset"].size();
        }
    } catch (...) {}
    return 0;
}

int yaml_get_param_count(int pset_idx) {
    try {
        auto& pset = root_node["paramset"][pset_idx];
        if (pset.contains("param") && pset["param"].is_sequence()) {
            return (int)pset["param"].size();
        }
    } catch (...) {}
    return 0;
}

void yaml_get_paramset_string(int pset_idx, const char* key, char* out, int out_len) {
    std::string val = "";
    try {
        auto& pset = root_node["paramset"][pset_idx];
        if (pset.contains(key)) {
            if (pset[key].is_string()) {
                val = pset[key].get_value<std::string>();
            } else if (pset[key].is_integer()) {
                val = std::to_string(pset[key].get_value<long>());
            } else if (pset[key].is_float()) {
                val = std::to_string(pset[key].get_value<double>());
            }
        }
    } catch (...) {}

    std::strncpy(out, val.c_str(), out_len - 1);
    out[std::min((int)val.length(), out_len - 1)] = '\0';
}

int yaml_get_paramset_int(int pset_idx, const char* key, int default_val) {
    try {
        auto& pset = root_node["paramset"][pset_idx];
        if (pset.contains(key)) {
            if (pset[key].is_integer()) {
                return (int)pset[key].get_value<long>();
            } else if (pset[key].is_string()) {
                std::string s = pset[key].get_value<std::string>();
                if (s == "?") return default_val;
                return std::stoi(s);
            }
        }
    } catch (...) {}
    return default_val;
}

void yaml_get_param_string(int pset_idx, int param_idx, const char* key, char* out, int out_len) {
    std::string val = "";
    try {
        auto& param = root_node["paramset"][pset_idx]["param"][param_idx];
        if (param.contains(key)) {
            if (param[key].is_string()) {
                val = param[key].get_value<std::string>();
            } else if (param[key].is_integer()) {
                val = std::to_string(param[key].get_value<long>());
            } else if (param[key].is_float()) {
                val = std::to_string(param[key].get_value<double>());
            }
        }
    } catch (...) {}

    std::strncpy(out, val.c_str(), out_len - 1);
    out[std::min((int)val.length(), out_len - 1)] = '\0';
}

int yaml_get_param_int(int pset_idx, int param_idx, const char* key, int default_val) {
    try {
        auto& param = root_node["paramset"][pset_idx]["param"][param_idx];
        if (param.contains(key)) {
            if (param[key].is_integer()) {
                return (int)param[key].get_value<long>();
            } else if (param[key].is_string()) {
                std::string s = param[key].get_value<std::string>();
                if (s == "?") return default_val;
                return std::stoi(s);
            }
        }
    } catch (...) {}
    return default_val;
}

double yaml_get_param_double(int pset_idx, int param_idx, const char* key, double default_val) {
    try {
        auto& param = root_node["paramset"][pset_idx]["param"][param_idx];
        if (param.contains(key)) {
            if (param[key].is_float()) {
                return param[key].get_value<double>();
            } else if (param[key].is_integer()) {
                return (double)param[key].get_value<long>();
            } else if (param[key].is_string()) {
                std::string s = param[key].get_value<std::string>();
                if (s == "?") return default_val;
                return std::stod(s);
            }
        }
    } catch (...) {}
    return default_val;
}

int yaml_get_param_array_size(int pset_idx, int param_idx, const char* key) {
    try {
        auto& param = root_node["paramset"][pset_idx]["param"][param_idx];
        if (param.contains(key)) {
            if (param[key].is_sequence()) {
                return (int)param[key].size();
            } else {
                return 1;
            }
        }
    } catch (...) {}
    return 0;
}

void yaml_get_param_array_float(int pset_idx, int param_idx, const char* key, float* out, int out_len) {
    try {
        auto& param = root_node["paramset"][pset_idx]["param"][param_idx];
        if (param.contains(key)) {
            if (param[key].is_sequence()) {
                for (int i = 0; i < out_len && i < (int)param[key].size(); ++i) {
                    if (param[key][i].is_float()) out[i] = (float)param[key][i].get_value<double>();
                    else if (param[key][i].is_integer()) out[i] = (float)param[key][i].get_value<long>();
                    else if (param[key][i].is_string()) {
                        std::string s = param[key][i].get_value<std::string>();
                        if (s == "?") out[i] = 0.0f;
                        else out[i] = std::stof(s);
                    }
                }
            } else {
                if (param[key].is_float()) out[0] = (float)param[key].get_value<double>();
                else if (param[key].is_integer()) out[0] = (float)param[key].get_value<long>();
                else if (param[key].is_string()) {
                    std::string s = param[key].get_value<std::string>();
                    if (s == "?") out[0] = 0.0f;
                    else out[0] = std::stof(s);
                }
            }
        }
    } catch (...) {}
}

void yaml_get_param_array_int(int pset_idx, int param_idx, const char* key, int* out, int out_len) {
    try {
        auto& param = root_node["paramset"][pset_idx]["param"][param_idx];
        if (param.contains(key)) {
            if (param[key].is_sequence()) {
                for (int i = 0; i < out_len && i < (int)param[key].size(); ++i) {
                    if (param[key][i].is_integer()) out[i] = (int)param[key][i].get_value<long>();
                    else if (param[key][i].is_string()) {
                        std::string s = param[key][i].get_value<std::string>();
                        if (s == "?") out[i] = 0;
                        else out[i] = std::stoi(s);
                    }
                }
            } else {
                if (param[key].is_integer()) out[0] = (int)param[key].get_value<long>();
                else if (param[key].is_string()) {
                    std::string s = param[key].get_value<std::string>();
                    if (s == "?") out[0] = 0;
                    else out[0] = std::stoi(s);
                }
            }
        }
    } catch (...) {}
}

}
