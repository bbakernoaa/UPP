#include <eckit/config/YAMLConfiguration.h>
#include <eckit/config/LocalConfiguration.h>
#include <eckit/filesystem/PathName.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <memory>
#include "yaml_reader.h"

static std::unique_ptr<eckit::YAMLConfiguration> root_config;

extern "C" {

int yaml_load_file(const char* filename) {
    try {
        root_config = std::make_unique<eckit::YAMLConfiguration>(eckit::PathName(filename));
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "YAML Load Error: " << e.what() << std::endl;
        return -2;
    }
}

void yaml_free() {
    root_config.reset();
}

int yaml_get_paramset_count() {
    if (!root_config) return 0;
    try {
        if (root_config->has("paramset")) {
            std::vector<eckit::LocalConfiguration> psets = root_config->getSubConfigurations("paramset");
            return (int)psets.size();
        }
    } catch (...) {}
    return 0;
}

int yaml_get_param_count(int pset_idx) {
    if (!root_config) return 0;
    try {
        std::vector<eckit::LocalConfiguration> psets = root_config->getSubConfigurations("paramset");
        if (pset_idx >= 0 && pset_idx < (int)psets.size()) {
            if (psets[pset_idx].has("param")) {
                std::vector<eckit::LocalConfiguration> params = psets[pset_idx].getSubConfigurations("param");
                return (int)params.size();
            }
        }
    } catch (...) {}
    return 0;
}

void yaml_get_paramset_string(int pset_idx, const char* key, char* out, int out_len) {
    std::string val = "";
    if (root_config) {
        try {
            std::vector<eckit::LocalConfiguration> psets = root_config->getSubConfigurations("paramset");
            if (pset_idx >= 0 && pset_idx < (int)psets.size()) {
                if (psets[pset_idx].has(key)) {
                    val = psets[pset_idx].getString(key);
                }
            }
        } catch (...) {}
    }

    std::strncpy(out, val.c_str(), out_len - 1);
    out[std::min((int)val.length(), out_len - 1)] = '\0';
}

int yaml_get_paramset_int(int pset_idx, const char* key, int default_val) {
    if (!root_config) return default_val;
    try {
        std::vector<eckit::LocalConfiguration> psets = root_config->getSubConfigurations("paramset");
        if (pset_idx >= 0 && pset_idx < (int)psets.size()) {
            if (psets[pset_idx].has(key)) {
                std::string s = psets[pset_idx].getString(key);
                if (s == "?") return default_val;
                return psets[pset_idx].getInt(key);
            }
        }
    } catch (...) {}
    return default_val;
}

void yaml_get_param_string(int pset_idx, int param_idx, const char* key, char* out, int out_len) {
    std::string val = "";
    if (root_config) {
        try {
            std::vector<eckit::LocalConfiguration> psets = root_config->getSubConfigurations("paramset");
            if (pset_idx >= 0 && pset_idx < (int)psets.size()) {
                std::vector<eckit::LocalConfiguration> params = psets[pset_idx].getSubConfigurations("param");
                if (param_idx >= 0 && param_idx < (int)params.size()) {
                    if (params[param_idx].has(key)) {
                        val = params[param_idx].getString(key);
                    }
                }
            }
        } catch (...) {}
    }

    std::strncpy(out, val.c_str(), out_len - 1);
    out[std::min((int)val.length(), out_len - 1)] = '\0';
}

int yaml_get_param_int(int pset_idx, int param_idx, const char* key, int default_val) {
    if (!root_config) return default_val;
    try {
        std::vector<eckit::LocalConfiguration> psets = root_config->getSubConfigurations("paramset");
        if (pset_idx >= 0 && pset_idx < (int)psets.size()) {
            std::vector<eckit::LocalConfiguration> params = psets[pset_idx].getSubConfigurations("param");
            if (param_idx >= 0 && param_idx < (int)params.size()) {
                if (params[param_idx].has(key)) {
                    std::string s = params[param_idx].getString(key);
                    if (s == "?") return default_val;
                    return params[param_idx].getInt(key);
                }
            }
        }
    } catch (...) {}
    return default_val;
}

double yaml_get_param_double(int pset_idx, int param_idx, const char* key, double default_val) {
    if (!root_config) return default_val;
    try {
        std::vector<eckit::LocalConfiguration> psets = root_config->getSubConfigurations("paramset");
        if (pset_idx >= 0 && pset_idx < (int)psets.size()) {
            std::vector<eckit::LocalConfiguration> params = psets[pset_idx].getSubConfigurations("param");
            if (param_idx >= 0 && param_idx < (int)params.size()) {
                if (params[param_idx].has(key)) {
                    std::string s = params[param_idx].getString(key);
                    if (s == "?") return default_val;
                    return params[param_idx].getDouble(key);
                }
            }
        }
    } catch (...) {}
    return default_val;
}

int yaml_get_param_array_size(int pset_idx, int param_idx, const char* key) {
    if (!root_config) return 0;
    try {
        std::vector<eckit::LocalConfiguration> psets = root_config->getSubConfigurations("paramset");
        if (pset_idx >= 0 && pset_idx < (int)psets.size()) {
            std::vector<eckit::LocalConfiguration> params = psets[pset_idx].getSubConfigurations("param");
            if (param_idx >= 0 && param_idx < (int)params.size()) {
                if (params[param_idx].has(key)) {
                    // eckit doesn't have a direct is_sequence, but we can try to get it as a vector
                    try {
                        std::vector<std::string> v = params[param_idx].getStringVector(key);
                        return (int)v.size();
                    } catch (...) {
                        return 1;
                    }
                }
            }
        }
    } catch (...) {}
    return 0;
}

void yaml_get_param_array_float(int pset_idx, int param_idx, const char* key, float* out, int out_len) {
    if (!root_config) return;
    try {
        std::vector<eckit::LocalConfiguration> psets = root_config->getSubConfigurations("paramset");
        if (pset_idx >= 0 && pset_idx < (int)psets.size()) {
            std::vector<eckit::LocalConfiguration> params = psets[pset_idx].getSubConfigurations("param");
            if (param_idx >= 0 && param_idx < (int)params.size()) {
                if (params[param_idx].has(key)) {
                    try {
                        std::vector<double> v = params[param_idx].getDoubleVector(key);
                        for (int i = 0; i < out_len && i < (int)v.size(); ++i) {
                            out[i] = (float)v[i];
                        }
                    } catch (...) {
                        // Try reading as string vector to handle "?"
                        try {
                            std::vector<std::string> vs = params[param_idx].getStringVector(key);
                            for (int i = 0; i < out_len && i < (int)vs.size(); ++i) {
                                if (vs[i] == "?") out[i] = 0.0f;
                                else out[i] = std::stof(vs[i]);
                            }
                        } catch (...) {
                           // Single value
                           std::string s = params[param_idx].getString(key);
                           if (s == "?") out[0] = 0.0f;
                           else out[0] = (float)params[param_idx].getDouble(key);
                        }
                    }
                }
            }
        }
    } catch (...) {}
}

void yaml_get_param_array_int(int pset_idx, int param_idx, const char* key, int* out, int out_len) {
    if (!root_config) return;
    try {
        std::vector<eckit::LocalConfiguration> psets = root_config->getSubConfigurations("paramset");
        if (pset_idx >= 0 && pset_idx < (int)psets.size()) {
            std::vector<eckit::LocalConfiguration> params = psets[pset_idx].getSubConfigurations("param");
            if (param_idx >= 0 && param_idx < (int)params.size()) {
                if (params[param_idx].has(key)) {
                    try {
                        std::vector<int> v = params[param_idx].getIntVector(key);
                        for (int i = 0; i < out_len && i < (int)v.size(); ++i) {
                            out[i] = v[i];
                        }
                    } catch (...) {
                         // Try reading as string vector to handle "?"
                        try {
                            std::vector<std::string> vs = params[param_idx].getStringVector(key);
                            for (int i = 0; i < out_len && i < (int)vs.size(); ++i) {
                                if (vs[i] == "?") out[i] = 0;
                                else out[i] = std::stoi(vs[i]);
                            }
                        } catch (...) {
                           // Single value
                           std::string s = params[param_idx].getString(key);
                           if (s == "?") out[0] = 0;
                           else out[0] = params[param_idx].getInt(key);
                        }
                    }
                }
            }
        }
    } catch (...) {}
}

}
