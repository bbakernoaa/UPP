#include <eckit/config/YAMLConfiguration.h>
#include <eckit/config/LocalConfiguration.h>
#include <eckit/filesystem/PathName.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include <algorithm>
#include "yaml_reader.h"

static std::unique_ptr<eckit::YAMLConfiguration> root_config;
static std::vector<eckit::LocalConfiguration> cached_psets;
static int cached_pset_idx = -1;
static std::vector<eckit::LocalConfiguration> cached_params;

extern "C" {

int yaml_load_file(const char* filename) {
    try {
        root_config = std::make_unique<eckit::YAMLConfiguration>(eckit::PathName(filename));
        cached_psets.clear();
        if (root_config->has("paramset")) {
            cached_psets = root_config->getSubConfigurations("paramset");
        }
        cached_pset_idx = -1;
        cached_params.clear();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "YAML Load Error: " << e.what() << std::endl;
        return -2;
    }
}

void yaml_free() {
    root_config.reset();
    cached_psets.clear();
    cached_params.clear();
    cached_pset_idx = -1;
}

int yaml_get_paramset_count() {
    return (int)cached_psets.size();
}

int yaml_get_param_count(int pset_idx) {
    if (pset_idx < 0 || pset_idx >= (int)cached_psets.size()) return 0;
    try {
        if (pset_idx != cached_pset_idx) {
            cached_pset_idx = pset_idx;
            cached_params.clear();
            if (cached_psets[pset_idx].has("param")) {
                cached_params = cached_psets[pset_idx].getSubConfigurations("param");
            }
        }
        return (int)cached_params.size();
    } catch (...) {}
    return 0;
}

void yaml_get_paramset_string(int pset_idx, const char* key, char* out, int out_len) {
    std::string val = "";
    if (pset_idx >= 0 && pset_idx < (int)cached_psets.size()) {
        try {
            if (cached_psets[pset_idx].has(key)) {
                val = cached_psets[pset_idx].getString(key);
            }
        } catch (...) {}
    }

    std::strncpy(out, val.c_str(), out_len - 1);
    out[std::min((int)val.length(), out_len - 1)] = '\0';
}

int yaml_get_paramset_int(int pset_idx, const char* key, int default_val) {
    if (pset_idx < 0 || pset_idx >= (int)cached_psets.size()) return default_val;
    try {
        if (cached_psets[pset_idx].has(key)) {
            std::string s = cached_psets[pset_idx].getString(key);
            if (s == "?") return default_val;
            return cached_psets[pset_idx].getInt(key);
        }
    } catch (...) {}
    return default_val;
}

void yaml_get_param_string(int pset_idx, int param_idx, const char* key, char* out, int out_len) {
    std::string val = "";
    if (pset_idx >= 0 && pset_idx < (int)cached_psets.size()) {
        try {
            if (pset_idx != cached_pset_idx) {
                cached_pset_idx = pset_idx;
                cached_params.clear();
                if (cached_psets[pset_idx].has("param")) {
                    cached_params = cached_psets[pset_idx].getSubConfigurations("param");
                }
            }
            if (param_idx >= 0 && param_idx < (int)cached_params.size()) {
                if (cached_params[param_idx].has(key)) {
                    val = cached_params[param_idx].getString(key);
                }
            }
        } catch (...) {}
    }

    std::strncpy(out, val.c_str(), out_len - 1);
    out[std::min((int)val.length(), out_len - 1)] = '\0';
}

int yaml_get_param_int(int pset_idx, int param_idx, const char* key, int default_val) {
    if (pset_idx < 0 || pset_idx >= (int)cached_psets.size()) return default_val;
    try {
        if (pset_idx != cached_pset_idx) {
            cached_pset_idx = pset_idx;
            cached_params.clear();
            if (cached_psets[pset_idx].has("param")) {
                cached_params = cached_psets[pset_idx].getSubConfigurations("param");
            }
        }
        if (param_idx >= 0 && param_idx < (int)cached_params.size()) {
            if (cached_params[param_idx].has(key)) {
                std::string s = cached_params[param_idx].getString(key);
                if (s == "?") return default_val;
                return cached_params[param_idx].getInt(key);
            }
        }
    } catch (...) {}
    return default_val;
}

double yaml_get_param_double(int pset_idx, int param_idx, const char* key, double default_val) {
    if (pset_idx < 0 || pset_idx >= (int)cached_psets.size()) return default_val;
    try {
        if (pset_idx != cached_pset_idx) {
            cached_pset_idx = pset_idx;
            cached_params.clear();
            if (cached_psets[pset_idx].has("param")) {
                cached_params = cached_psets[pset_idx].getSubConfigurations("param");
            }
        }
        if (param_idx >= 0 && param_idx < (int)cached_params.size()) {
            if (cached_params[param_idx].has(key)) {
                std::string s = cached_params[param_idx].getString(key);
                if (s == "?") return default_val;
                return cached_params[param_idx].getDouble(key);
            }
        }
    } catch (...) {}
    return default_val;
}

int yaml_get_param_array_size(int pset_idx, int param_idx, const char* key) {
    if (pset_idx < 0 || pset_idx >= (int)cached_psets.size()) return 0;
    try {
        if (pset_idx != cached_pset_idx) {
            cached_pset_idx = pset_idx;
            cached_params.clear();
            if (cached_psets[pset_idx].has("param")) {
                cached_params = cached_psets[pset_idx].getSubConfigurations("param");
            }
        }
        if (param_idx >= 0 && param_idx < (int)cached_params.size()) {
            if (cached_params[param_idx].has(key)) {
                try {
                    std::vector<std::string> v = cached_params[param_idx].getStringVector(key);
                    return (int)v.size();
                } catch (...) {
                    return 1;
                }
            }
        }
    } catch (...) {}
    return 0;
}

void yaml_get_param_array_float(int pset_idx, int param_idx, const char* key, float* out, int out_len) {
    if (pset_idx < 0 || pset_idx >= (int)cached_psets.size()) return;
    try {
        if (pset_idx != cached_pset_idx) {
            cached_pset_idx = pset_idx;
            cached_params.clear();
            if (cached_psets[pset_idx].has("param")) {
                cached_params = cached_psets[pset_idx].getSubConfigurations("param");
            }
        }
        if (param_idx >= 0 && param_idx < (int)cached_params.size()) {
            if (cached_params[param_idx].has(key)) {
                try {
                    std::vector<double> v = cached_params[param_idx].getDoubleVector(key);
                    for (int i = 0; i < out_len && i < (int)v.size(); ++i) {
                        out[i] = (float)v[i];
                    }
                } catch (...) {
                    try {
                        std::vector<std::string> vs = cached_params[param_idx].getStringVector(key);
                        for (int i = 0; i < out_len && i < (int)vs.size(); ++i) {
                            if (vs[i] == "?") out[i] = 0.0f;
                            else out[i] = std::stof(vs[i]);
                        }
                    } catch (...) {
                       std::string s = cached_params[param_idx].getString(key);
                       if (s == "?") out[0] = 0.0f;
                       else out[0] = (float)cached_params[param_idx].getDouble(key);
                    }
                }
            }
        }
    } catch (...) {}
}

void yaml_get_param_array_int(int pset_idx, int param_idx, const char* key, int* out, int out_len) {
    if (pset_idx < 0 || pset_idx >= (int)cached_psets.size()) return;
    try {
        if (pset_idx != cached_pset_idx) {
            cached_pset_idx = pset_idx;
            cached_params.clear();
            if (cached_psets[pset_idx].has("param")) {
                cached_params = cached_psets[pset_idx].getSubConfigurations("param");
            }
        }
        if (param_idx >= 0 && param_idx < (int)cached_params.size()) {
            if (cached_params[param_idx].has(key)) {
                try {
                    std::vector<int> v = cached_params[param_idx].getIntVector(key);
                    for (int i = 0; i < out_len && i < (int)v.size(); ++i) {
                        out[i] = v[i];
                    }
                } catch (...) {
                    try {
                        std::vector<std::string> vs = cached_params[param_idx].getStringVector(key);
                        for (int i = 0; i < out_len && i < (int)vs.size(); ++i) {
                            if (vs[i] == "?") out[i] = 0;
                            else out[i] = std::stoi(vs[i]);
                        }
                    } catch (...) {
                       std::string s = cached_params[param_idx].getString(key);
                       if (s == "?") out[0] = 0;
                       else out[0] = cached_params[param_idx].getInt(key);
                    }
                }
            }
        }
    } catch (...) {}
}

}
