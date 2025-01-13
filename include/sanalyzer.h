#ifndef YOSEMITE_H
#define YOSEMITE_H

#include <cstddef>
#include <cstdint>
#include <string>

typedef enum {
    YOSEMITE_SUCCESS = 0,
    YOSEMITE_ERROR = 1,
    YOSEMITE_NOT_IMPLEMENTED = 2,
    YOSEMITE_CUDA_MEMFREE_ZERO = 3
} YosemiteResult_t;


typedef enum {
    GPU_NO_PATCH = 0,
    GPU_PATCH_APP_METRIC = 1,
    GPU_PATCH_MEM_TRACE = 2,
} SanitizerPatchName_t;


typedef struct SanitizerOptions {
    SanitizerPatchName_t patch_name;
    std::string patch_file;
    bool sanitizer_callback_enabled = true;
    bool torch_prof_enabled = false;

    SanitizerOptions() = default;
    ~SanitizerOptions() = default;
} SanitizerOptions_t;


YosemiteResult_t yosemite_alloc_callback(uint64_t ptr, uint64_t size, int type);

YosemiteResult_t yosemite_free_callback(uint64_t ptr, uint64_t size);

YosemiteResult_t yosemite_memcpy_callback(uint64_t dst, uint64_t src, uint64_t size, bool is_async, uint32_t direction);

YosemiteResult_t yosemite_memset_callback(uint64_t dst, uint32_t size, int value, bool is_async);

YosemiteResult_t yosemite_kernel_start_callback(std::string kernel_name);

YosemiteResult_t yosemite_kernel_end_callback(std::string kernel_name);

YosemiteResult_t yosemite_gpu_data_analysis(void* data, uint64_t size);

YosemiteResult_t yosemite_init(SanitizerOptions_t& options);

YosemiteResult_t yosemite_terminate();

YosemiteResult_t yosemite_tensor_malloc_callback(uint64_t ptr, int64_t alloc_size,
                                int64_t total_allocated, int64_t total_reserved);

YosemiteResult_t yosemite_tensor_free_callback(uint64_t ptr, int64_t alloc_size,
                                int64_t total_allocated, int64_t total_reserved);

YosemiteResult_t yosemite_query_active_ranges(void* ranges, uint32_t limit, uint32_t* count);


#endif // YOSEMITE_H
