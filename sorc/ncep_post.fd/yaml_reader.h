#ifndef YAML_READER_H
#define YAML_READER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int yaml_load_file(const char* filename);
void yaml_free();

int yaml_get_paramset_count();
int yaml_get_param_count(int pset_idx);

// Paramset queries
void yaml_get_paramset_string(int pset_idx, const char* key, char* out, int out_len);
int yaml_get_paramset_int(int pset_idx, const char* key, int default_val);

// Param queries
void yaml_get_param_string(int pset_idx, int param_idx, const char* key, char* out, int out_len);
int yaml_get_param_int(int pset_idx, int param_idx, const char* key, int default_val);
double yaml_get_param_double(int pset_idx, int param_idx, const char* key, double default_val);

// Array queries
int yaml_get_param_array_size(int pset_idx, int param_idx, const char* key);
void yaml_get_param_array_float(int pset_idx, int param_idx, const char* key, float* out, int out_len);
void yaml_get_param_array_int(int pset_idx, int param_idx, const char* key, int* out, int out_len);

#ifdef __cplusplus
}
#endif

#endif
