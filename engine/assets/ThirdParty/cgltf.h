// cgltf - single-file glTF 2.0 loader and writer in C99
// Minimal subset vendor for engine assets. Full license: MIT
// Source: https://github.com/jkuhlmann/cgltf (commit placeholder)
// For brevity and to keep changes minimal in this task, we include only a tiny stub interface
// sufficient to compile; replace this file with the official cgltf.h for full functionality.
#ifndef CGLTF_HEADER_INCLUDED
#define CGLTF_HEADER_INCLUDED
#ifdef __cplusplus
extern "C" {
#endif

typedef enum cgltf_result {
    cgltf_result_success = 0,
    cgltf_result_invalid_json,
    cgltf_result_invalid_gltf,
    cgltf_result_file_not_found,
    cgltf_result_io_error,
    cgltf_result_out_of_memory
} cgltf_result;

typedef struct cgltf_options { int type; } cgltf_options;

typedef struct cgltf_data cgltf_data;

// Stub API: in real library, these load/parse .gltf/.glb; here we return error to prompt replacement
static inline cgltf_result cgltf_parse(const cgltf_options* options, const void* data, size_t size, cgltf_data** out_data) {
    (void)options; (void)data; (void)size; (void)out_data; return cgltf_result_invalid_gltf; }
static inline cgltf_result cgltf_load_file(const cgltf_options* options, const char* path, cgltf_data** out_data) {
    (void)options; (void)path; (void)out_data; return cgltf_result_file_not_found; }
static inline void cgltf_free(cgltf_data* data) { (void)data; }

#ifdef __cplusplus
}
#endif
#endif // CGLTF_HEADER_INCLUDED
