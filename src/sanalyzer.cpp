#include "sanalyzer.h"

#include "tools/tool.h"
#include "utils/event.h"
#include "tools/code_check.h"
#include "tools/app_metric.h"
#include "tools/mem_trace.h"
#include "tools/hot_analysis.h"

#include <memory>
#include <map>
#include <iostream>

using namespace yosemite;

static std::map<AnalysisTool_t, std::shared_ptr<Tool>> _tools;


YosemiteResult_t yosemite_tool_enable(AnalysisTool_t& tool) {
    const char* tool_name = std::getenv("YOSEMITE_TOOL_NAME");
    if (!tool_name) {
        fprintf(stdout, "No tool name specified.\n");
        return YOSEMITE_NOT_IMPLEMENTED;
    }

    if (std::string(tool_name) == "code_check") {
        tool = CODE_CHECK;
        _tools.emplace(CODE_CHECK, std::make_shared<CodeCheck>());
    } else if (std::string(tool_name) == "app_metric") {
        tool = APP_METRICE;
        _tools.emplace(APP_METRICE, std::make_shared<AppMetrics>());
    } else if (std::string(tool_name) == "mem_trace") {
        tool = MEM_TRACE;
        _tools.emplace(MEM_TRACE, std::make_shared<MemTrace>());
    } else if (std::string(tool_name) == "hot_analysis") {
        tool = HOT_ANALYSIS;
        _tools.emplace(HOT_ANALYSIS, std::make_shared<HotAnalysis>());
    } else {
        fprintf(stdout, "Tool not found.\n");
        return YOSEMITE_NOT_IMPLEMENTED;
    }

    fprintf(stdout, "Enabling %s tool.\n", tool_name);
    fflush(stdout);
    return YOSEMITE_SUCCESS;
}


YosemiteResult_t yosemite_tool_disable() {
    return YOSEMITE_SUCCESS;
}


YosemiteResult_t yosemite_flush() {
    for (auto &tool : _tools) {
        tool.second->flush();
    }
    return YOSEMITE_SUCCESS;
}


YosemiteResult_t yosemite_torch_prof_enable() {
    fprintf(stdout, "Enabling torch profiler.\n");
    fflush(stdout);
    return YOSEMITE_SUCCESS;
}


/****************************************************************************************
 ********************************** Interface functions *********************************
****************************************************************************************/


YosemiteResult_t yosemite_alloc_callback(uint64_t ptr, uint64_t size, int type) {
    for (auto &tool : _tools) {
        auto mem_alloc = std::make_shared<MemAlloc_t>(ptr, size, type);
        tool.second->evt_callback(mem_alloc);
    }
    return YOSEMITE_SUCCESS;
}


YosemiteResult_t yosemite_free_callback(uint64_t ptr, uint64_t size, int type) {
    if (ptr == 0) {
        return YOSEMITE_CUDA_MEMFREE_ZERO;
    }
    for (auto &tool : _tools) {
        auto mem_free = std::make_shared<MemFree_t>(ptr, size, type);
        tool.second->evt_callback(mem_free);
    }
    return YOSEMITE_SUCCESS;
}


YosemiteResult_t yosemite_memcpy_callback(uint64_t dst, uint64_t src, uint64_t size, bool is_async, uint32_t direction) {
    for (auto &tool : _tools) {
        auto mem_cpy = std::make_shared<MemCpy_t>(dst, src, size, is_async, direction);
        tool.second->evt_callback(mem_cpy);
    }
    return YOSEMITE_SUCCESS;
}


YosemiteResult_t yosemite_memset_callback(uint64_t dst, uint32_t size, int value, bool is_async) {
    for (auto &tool : _tools) {
        auto mem_set = std::make_shared<MemSet_t>(dst, size, value, is_async);
        tool.second->evt_callback(mem_set);
    }
    return YOSEMITE_SUCCESS;
}


YosemiteResult_t yosemite_kernel_start_callback(std::string kernel_name) {
    for (auto &tool : _tools) {
        auto kernel = std::make_shared<KernelLauch_t>(kernel_name); 
        tool.second->evt_callback(kernel);
    }
    return YOSEMITE_SUCCESS;
}


YosemiteResult_t yosemite_kernel_end_callback(std::string kernel_name) {
    for (auto &tool : _tools) {
        auto kernel = std::make_shared<KernelEnd_t>(kernel_name);
        tool.second->evt_callback(kernel);
    }
    return YOSEMITE_SUCCESS;
}


YosemiteResult_t yosemite_gpu_data_analysis(void* data, uint64_t size) {
    for (auto &tool : _tools) {
        tool.second->gpu_data_analysis(data, size);
    }
    return YOSEMITE_SUCCESS;
}


YosemiteResult_t yosemite_init(SanitizerOptions_t& options) {
    AnalysisTool_t tool;
    YosemiteResult_t res = yosemite_tool_enable(tool);
    if (res != YOSEMITE_SUCCESS) {
        return res;
    }

    if (tool == CODE_CHECK) {
        options.patch_name = GPU_NO_PATCH;
    } else if (tool == APP_METRICE) {
        options.patch_name = GPU_PATCH_APP_METRIC;
        options.patch_file = "gpu_patch_app_metric.fatbin";
    } else if (tool == MEM_TRACE) {
        options.patch_name = GPU_PATCH_MEM_TRACE;
        options.patch_file = "gpu_patch_mem_trace.fatbin";
    } else if (tool == HOT_ANALYSIS) {
        options.patch_name = GPU_PATCH_HOT_ANALYSIS;
        options.patch_file = "gpu_patch_hot_analysis.fatbin";
    }

    // enable torch profiler?
    const char* torch_prof = std::getenv("TORCH_PROFILE_ENABLED");
    if (torch_prof && std::string(torch_prof) == "1") {
        options.torch_prof_enabled = true;
        yosemite_torch_prof_enable();
    }

    fprintf(stdout, "================================================================================\n");
    fflush(stdout);

    return YOSEMITE_SUCCESS;
}


YosemiteResult_t yosemite_terminate() {
    yosemite_flush();
    return YOSEMITE_SUCCESS;
}


YosemiteResult_t yosemite_tensor_malloc_callback(uint64_t ptr, int64_t alloc_size,
                                    int64_t total_allocated, int64_t total_reserved) {
    for (auto &tool : _tools) {
        auto ten_alloc = std::make_shared<TenAlloc_t>(ptr, alloc_size, total_allocated, total_reserved);
        tool.second->evt_callback(ten_alloc);
    }
    return YOSEMITE_SUCCESS;
}


YosemiteResult_t yosemite_tensor_free_callback(uint64_t ptr, int64_t alloc_size,
                                    int64_t total_allocated, int64_t total_reserved) {
    for (auto &tool : _tools) {
        auto ten_free = std::make_shared<TenFree_t>(ptr, alloc_size, total_allocated, total_reserved);
        tool.second->evt_callback(ten_free);
    }
    return YOSEMITE_SUCCESS;
}


YosemiteResult_t yosemite_query_active_ranges(void* ranges, uint32_t limit, uint32_t* count) {
    for (auto &tool : _tools) {
        tool.second->query_ranges(ranges, limit, count);
    }
    return YOSEMITE_SUCCESS;
}
