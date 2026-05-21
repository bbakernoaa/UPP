#include "yaml_reader.h"
#include <fkYAML/node.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>

static fkyaml::node root_node;
static bool is_loaded = false;

extern "C" {

int yaml_load_file(const char* filename) {
    try {
        std::ifstream ifs(filename);
        if (!ifs.is_open()) {
            return -1;
        }
        root_node = fkyaml::node::deserialize(ifs);
        is_loaded = true;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error loading YAML: " << e.what() << std::endl;
        return -2;
    }
}

void yaml_free() {
    root_node = fkyaml::node();
    is_loaded = false;
}

int yaml_get_paramset_count() {
    if (!is_loaded || !root_node.contains("paramset")) return 0;
    return root_node["paramset"].size();
}

int yaml_get_param_count(int pset_idx) {
    if (!is_loaded || !root_node.contains("paramset")) return 0;
    auto& pset = root_node["paramset"][pset_idx];
    if (!pset.contains("param")) return 0;
    return pset["param"].size();
}

static void copy_string(const std::string& val, char* out, int out_len) {
    int len = std::min((int)val.length(), out_len - 1);
    std::memcpy(out, val.c_str(), len);
    out[len] = '\0';
}

void yaml_get_paramset_string(int pset_idx, const char* key, char* out, int out_len) {
    if (!is_loaded) return;
    try {
        auto& pset = root_node["paramset"][pset_idx];
        if (pset.contains(key)) {
            std::string val = pset[key].get_value<std::string>();
            copy_string(val, out, out_len);
        } else {
            copy_string("?", out, out_len);
        }
    } catch (...) {
        copy_string("?", out, out_len);
    }
}

int yaml_get_paramset_int(int pset_idx, const char* key, int default_val) {
    if (!is_loaded) return default_val;
    try {
        auto& pset = root_node["paramset"][pset_idx];
        if (pset.contains(key)) {
            return pset[key].get_value<int>();
        }
    } catch (...) {}
    return default_val;
}

void yaml_get_param_string(int pset_idx, int param_idx, const char* key, char* out, int out_len) {
    if (!is_loaded) return;
    try {
        auto& pset = root_node["paramset"][pset_idx];
        auto& param = pset["param"][param_idx];
        if (param.contains(key)) {
            std::string val = param[key].get_value<std::string>();
            copy_string(val, out, out_len);
        } else {
            copy_string("?", out, out_len);
        }
    } catch (...) {
        copy_string("?", out, out_len);
    }
}

int yaml_get_param_int(int pset_idx, int param_idx, const char* key, int default_val) {
    if (!is_loaded) return default_val;
    try {
        auto& pset = root_node["paramset"][pset_idx];
        auto& param = pset["param"][param_idx];
        if (param.contains(key)) {
            return param[key].get_value<int>();
        }
    } catch (...) {}
    return default_val;
}

double yaml_get_param_double(int pset_idx, int param_idx, const char* key, double default_val) {
    if (!is_loaded) return default_val;
    try {
        auto& pset = root_node["paramset"][pset_idx];
        auto& param = pset["param"][param_idx];
        if (param.contains(key)) {
            return param[key].get_value<double>();
        }
    } catch (...) {}
    return default_val;
}

int yaml_get_param_array_size(int pset_idx, int param_idx, const char* key) {
    if (!is_loaded) return 0;
    try {
        auto& pset = root_node["paramset"][pset_idx];
        auto& param = pset["param"][param_idx];
        if (param.contains(key) && param[key].is_sequence()) {
            return param[key].size();
        }
    } catch (...) {}
    return 0;
}

void yaml_get_param_array_float(int pset_idx, int param_idx, const char* key, float* out, int out_len) {
    if (!is_loaded) return;
    try {
        auto& pset = root_node["paramset"][pset_idx];
        auto& param = pset["param"][param_idx];
        if (param.contains(key) && param[key].is_sequence()) {
            int n = param[key].size();
            for (int i = 0; i < n && i < out_len; ++i) {
                out[i] = (float)param[key][i].get_value<double>();
            }
        }
    } catch (...) {}
}

void yaml_get_param_array_int(int pset_idx, int param_idx, const char* key, int* out, int out_len) {
    if (!is_loaded) return;
    try {
        auto& pset = root_node["paramset"][pset_idx];
        auto& param = pset["param"][param_idx];
        if (param.contains(key) && param[key].is_sequence()) {
            int n = param[key].size();
            for (int i = 0; i < n && i < out_len; ++i) {
                out[i] = param[key][i].get_value<int>();
            }
        }
    } catch (...) {}
}

}
