#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include "yaml_reader.h"

extern "C" {

int yaml_load_file(const char* filename) {
    std::cout << "yaml_reader stub: Attempted to load file: " << (filename ? filename : "null") << std::endl;
    return -1; // File loading is bypassed under modernized driver
}

void yaml_free() {}

int yaml_get_paramset_count() { return 0; }

int yaml_get_param_count(int pset_idx) { (void)pset_idx; return 0; }

void yaml_get_paramset_string(int pset_idx, const char* key, char* out, int out_len) {
    (void)pset_idx; (void)key;
    if (out && out_len > 0) out[0] = '\0';
}

int yaml_get_paramset_int(int pset_idx, const char* key, int default_val) {
    (void)pset_idx; (void)key;
    return default_val;
}

void yaml_get_param_string(int pset_idx, int param_idx, const char* key, char* out, int out_len) {
    (void)pset_idx; (void)param_idx; (void)key;
    if (out && out_len > 0) out[0] = '\0';
}

int yaml_get_param_int(int pset_idx, int param_idx, const char* key, int default_val) {
    (void)pset_idx; (void)param_idx; (void)key;
    return default_val;
}

double yaml_get_param_double(int pset_idx, int param_idx, const char* key, double default_val) {
    (void)pset_idx; (void)param_idx; (void)key;
    return default_val;
}

int yaml_get_param_array_size(int pset_idx, int param_idx, const char* key) {
    (void)pset_idx; (void)param_idx; (void)key;
    return 0;
}

void yaml_get_param_array_float(int pset_idx, int param_idx, const char* key, float* out, int out_len) {
    (void)pset_idx; (void)param_idx; (void)key; (void)out; (void)out_len;
}

void yaml_get_param_array_int(int pset_idx, int param_idx, const char* key, int* out, int out_len) {
    (void)pset_idx; (void)param_idx; (void)key; (void)out; (void)out_len;
}

}
