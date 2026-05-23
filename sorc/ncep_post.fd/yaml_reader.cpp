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

static std::string get_string_flexible(const eckit::LocalConfiguration& conf, const std::string& key) {
    if (!conf.has(key)) return "";
    try { return conf.getString(key); } catch (...) {}
    try { return std::to_string(conf.getInt(key)); } catch (...) {}
    try { return std::to_string(conf.getDouble(key)); } catch (...) {}
    return "";
}

static int get_int_flexible(const eckit::LocalConfiguration& conf, const std::string& key, int default_val) {
    if (!conf.has(key)) return default_val;
    try { return conf.getInt(key); } catch (...) {}
    try {
        std::string s = conf.getString(key);
        if (s == "?") return default_val;
        return std::stoi(s);
    } catch (...) {}
    return default_val;
}

static double get_double_flexible(const eckit::LocalConfiguration& conf, const std::string& key, double default_val) {
    if (!conf.has(key)) return default_val;
    try { return conf.getDouble(key); } catch (...) {}
    try { return (double)conf.getInt(key); } catch (...) {}
    try {
        std::string s = conf.getString(key);
        if (s == "?") return default_val;
        return std::stod(s);
    } catch (...) {}
    return default_val;
}

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
        val = get_string_flexible(cached_psets[pset_idx], key);
    }
    std::strncpy(out, val.c_str(), out_len - 1);
    out[std::min((int)val.length(), out_len - 1)] = '\0';
}

int yaml_get_paramset_int(int pset_idx, const char* key, int default_val) {
    if (pset_idx < 0 || pset_idx >= (int)cached_psets.size()) return default_val;
    return get_int_flexible(cached_psets[pset_idx], key, default_val);
}

void yaml_get_param_string(int pset_idx, int param_idx, const char* key, char* out, int out_len) {
    std::string val = "";
    if (pset_idx >= 0 && pset_idx < (int)cached_psets.size()) {
        if (pset_idx != cached_pset_idx) {
            cached_pset_idx = pset_idx;
            cached_params.clear();
            if (cached_psets[pset_idx].has("param")) {
                cached_params = cached_psets[pset_idx].getSubConfigurations("param");
            }
        }
        if (param_idx >= 0 && param_idx < (int)cached_params.size()) {
            val = get_string_flexible(cached_params[param_idx], key);
        }
    }
    std::strncpy(out, val.c_str(), out_len - 1);
    out[std::min((int)val.length(), out_len - 1)] = '\0';
}

int yaml_get_param_int(int pset_idx, int param_idx, const char* key, int default_val) {
    if (pset_idx < 0 || pset_idx >= (int)cached_psets.size()) return default_val;
    if (pset_idx != cached_pset_idx) {
        cached_pset_idx = pset_idx;
        cached_params.clear();
        if (cached_psets[pset_idx].has("param")) {
            cached_params = cached_psets[pset_idx].getSubConfigurations("param");
        }
    }
    if (param_idx >= 0 && param_idx < (int)cached_params.size()) {
        return get_int_flexible(cached_params[param_idx], key, default_val);
    }
    return default_val;
}

double yaml_get_param_double(int pset_idx, int param_idx, const char* key, double default_val) {
    if (pset_idx < 0 || pset_idx >= (int)cached_psets.size()) return default_val;
    if (pset_idx != cached_pset_idx) {
        cached_pset_idx = pset_idx;
        cached_params.clear();
        if (cached_psets[pset_idx].has("param")) {
            cached_params = cached_psets[pset_idx].getSubConfigurations("param");
        }
    }
    if (param_idx >= 0 && param_idx < (int)cached_params.size()) {
        return get_double_flexible(cached_params[param_idx], key, default_val);
    }
    return default_val;
}

int yaml_get_param_array_size(int pset_idx, int param_idx, const char* key) {
    if (pset_idx < 0 || pset_idx >= (int)cached_psets.size()) return 0;
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
                try {
                    std::vector<long> v = cached_params[param_idx].getIntVector(key);
                    return (int)v.size();
                } catch (...) {
                    try {
                        std::vector<double> v = cached_params[param_idx].getDoubleVector(key);
                        return (int)v.size();
                    } catch (...) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

void yaml_get_param_array_float(int pset_idx, int param_idx, const char* key, float* out, int out_len) {
    if (pset_idx < 0 || pset_idx >= (int)cached_psets.size()) return;
    if (pset_idx != cached_pset_idx) {
        cached_pset_idx = pset_idx;
        cached_params.clear();
        if (cached_psets[pset_idx].has("param")) {
            cached_params = cached_psets[pset_idx].getSubConfigurations("param");
        }
    }
    if (param_idx >= 0 && param_idx < (int)cached_params.size()) {
        const auto& param = cached_params[param_idx];
        if (param.has(key)) {
            try {
                std::vector<double> v = param.getDoubleVector(key);
                for (int i = 0; i < out_len && i < (int)v.size(); ++i) out[i] = (float)v[i];
            } catch (...) {
                try {
                    std::vector<long> v = param.getIntVector(key);
                    for (int i = 0; i < out_len && i < (int)v.size(); ++i) out[i] = (float)v[i];
                } catch (...) {
                    try {
                        std::vector<std::string> vs = param.getStringVector(key);
                        for (int i = 0; i < out_len && i < (int)vs.size(); ++i) {
                            if (vs[i] == "?") out[i] = 0.0f;
                            else out[i] = std::stof(vs[i]);
                        }
                    } catch (...) {
                       out[0] = (float)get_double_flexible(param, key, 0.0);
                    }
                }
            }
        }
    }
}

void yaml_get_param_array_int(int pset_idx, int param_idx, const char* key, int* out, int out_len) {
    if (pset_idx < 0 || pset_idx >= (int)cached_psets.size()) return;
    if (pset_idx != cached_pset_idx) {
        cached_pset_idx = pset_idx;
        cached_params.clear();
        if (cached_psets[pset_idx].has("param")) {
            cached_params = cached_psets[pset_idx].getSubConfigurations("param");
        }
    }
    if (param_idx >= 0 && param_idx < (int)cached_params.size()) {
        const auto& param = cached_params[param_idx];
        if (param.has(key)) {
            try {
                std::vector<long> v = param.getIntVector(key);
                for (int i = 0; i < out_len && i < (int)v.size(); ++i) out[i] = (int)v[i];
            } catch (...) {
                try {
                    std::vector<std::string> vs = param.getStringVector(key);
                    for (int i = 0; i < out_len && i < (int)vs.size(); ++i) {
                        if (vs[i] == "?") out[i] = 0;
                        else out[i] = std::stoi(vs[i]);
                    }
                } catch (...) {
                   out[0] = get_int_flexible(param, key, 0);
                }
            }
        }
    }
}

}
