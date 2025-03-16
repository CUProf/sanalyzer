#include "tools/code_check.h"
#include "utils/helper.h"
#include "utils/hash.h"
#include "gpu_patch.h"
#include "cpp_trace.h"
#include "py_frame.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <iostream>

using namespace yosemite;

typedef enum {
    MEMCPY_UNKNOWN = 0,
    MEMCPY_H2H = 1,
    MEMCPY_H2D = 2,
    MEMCPY_D2H = 3,
    MEMCPY_D2D = 4,
} MemcpyDirection_t;

static Timer_t _timer;

struct CpyStats {
    uint64_t count = 0;
    uint64_t size = 0;
};

struct SetStats {
    uint64_t count = 0;
    uint64_t size = 0;
};

struct MemStats {
    uint64_t alloc_count = 0;
    uint64_t alloc_size = 0;
    uint64_t free_count = 0;
    uint64_t free_size = 0;
};

struct TenStats {
    uint64_t alloc_count = 0;
    uint64_t alloc_size = 0;
    uint64_t free_count = 0;
    uint64_t free_size = 0;
};


std::map<MemcpyDirection_t, CpyStats> cpy_stats;
SetStats set_stats;
MemStats mem_stats;
TenStats ten_stats;
uint64_t kernel_count = 0;


std::string vector2str(std::vector<std::string> &vec, int skip_first = 0, int skip_last = 0) {
    if (skip_first + skip_last > vec.size()) {
        printf("Skip first and skip last are larger than the vector size\n");
        return "";
    }
    std::string str;
    for (size_t i = skip_first; i < vec.size() - skip_last; i++) {
        str += vec[i] + "\n";
    }
    return str;
}

void CodeCheck::init() {
    const char* env_name = std::getenv("ACCEL_PROF_HOME");
    std::string lib_path;
    if (env_name) {
        lib_path = std::string(env_name) + "/lib/libcompute_sanitizer.so";
    }
    init_backtrace(lib_path.c_str());

}


void CodeCheck::evt_callback(EventPtr_t evt) {
    switch (evt->evt_type) {
        case EventType_KERNEL_LAUNCH:
            kernel_start_callback(std::dynamic_pointer_cast<KernelLauch_t>(evt));
            break;
        case EventType_KERNEL_END:
            kernel_end_callback(std::dynamic_pointer_cast<KernelEnd_t>(evt));
            break;
        case EventType_MEM_ALLOC:
            mem_alloc_callback(std::dynamic_pointer_cast<MemAlloc_t>(evt));
            break;
        case EventType_MEM_FREE:
            mem_free_callback(std::dynamic_pointer_cast<MemFree_t>(evt));
            break;
        case EventType_MEM_COPY:
            mem_cpy_callback(std::dynamic_pointer_cast<MemCpy_t>(evt));
            break;
        case EventType_MEM_SET:
            mem_set_callback(std::dynamic_pointer_cast<MemSet_t>(evt));
            break;
        case EventType_TEN_ALLOC:
            ten_alloc_callback(std::dynamic_pointer_cast<TenAlloc_t>(evt));
            break;
        case EventType_TEN_FREE:
            ten_free_callback(std::dynamic_pointer_cast<TenFree_t>(evt));
            break;
        default:
            break;
    }
}


void CodeCheck::kernel_start_callback(std::shared_ptr<KernelLauch_t> kernel) {
    kernel_count++;
    _timer.increment(true);
}


void CodeCheck::kernel_end_callback(std::shared_ptr<KernelEnd_t> kernel) {
}


void CodeCheck::mem_alloc_callback(std::shared_ptr<MemAlloc_t> mem) {
    mem_stats.alloc_count++;
    mem_stats.alloc_size += mem->size;

    _timer.increment(true);
}


void CodeCheck::mem_free_callback(std::shared_ptr<MemFree_t> mem) {
    mem_stats.free_count++;
    mem_stats.free_size += mem->size;

    _timer.increment(true);
}



void CodeCheck::mem_cpy_callback(std::shared_ptr<MemCpy_t> mem) {
    // auto backtraces = get_backtrace();
    // auto py_frames = get_pyframes();
    // auto bt_str = vector2str(backtraces);
    // auto pf_str = vector2str(py_frames);

    // std::cout << "Backtrace hash: " << sha256(bt_str) << std::endl;
    // std::cout << bt_str << std::endl;
    // std::cout << "Python frame hash: " << sha256(pf_str) << std::endl;
    // std::cout << pf_str << std::endl;

    MemcpyDirection_t direction = (MemcpyDirection_t)mem->direction;
    if (cpy_stats.find(direction) == cpy_stats.end()) {
        cpy_stats[direction] = CpyStats{0, 0};
    }
    cpy_stats[direction].count++;
    cpy_stats[direction].size += mem->size;

    _timer.increment(true);
}


void CodeCheck::mem_set_callback(std::shared_ptr<MemSet_t> mem) {
    set_stats.count++;
    set_stats.size += mem->size;

    _timer.increment(true);
}


void CodeCheck::ten_alloc_callback(std::shared_ptr<TenAlloc_t> ten) {
    ten_stats.alloc_count++;
    ten_stats.alloc_size += ten->size;

    _timer.increment(true);
}


void CodeCheck::ten_free_callback(std::shared_ptr<TenFree_t> ten) {
    ten_stats.free_count++;
    ten_stats.free_size += -ten->size;

    _timer.increment(true);
}


void CodeCheck::gpu_data_analysis(void* data, uint64_t size) {

}


void CodeCheck::query_ranges(void* ranges, uint32_t limit, uint32_t* count) {

}


void CodeCheck::flush() {
    fprintf(stdout, "--------------------------------------------------------------------------------\n");
    fprintf(stdout, "%-12s count: %-10lu\n", "[Kernel]", kernel_count);
    fprintf(stdout, "%-12s count: %-10lu, size: %lu (%s)\n", 
            "[MemMalloc]", mem_stats.alloc_count, mem_stats.alloc_size, format_size(mem_stats.alloc_size).c_str());
    fprintf(stdout, "%-12s count: %-10lu, size: %lu (%s)\n", 
            "[MemFree]", mem_stats.free_count, mem_stats.free_size, format_size(mem_stats.free_size).c_str());
    fprintf(stdout, "%-12s count: %-10lu, size: %lu (%s)\n", 
            "[Memset]", set_stats.count, set_stats.size, format_size(set_stats.size).c_str());

    for (auto& it : cpy_stats) {
        const char* direction = it.first == MEMCPY_H2H ? "H2H" : 
                              it.first == MEMCPY_H2D ? "H2D" :
                              it.first == MEMCPY_D2H ? "D2H" : 
                              it.first == MEMCPY_D2D ? "D2D" : "N/A";
        fprintf(stdout, "[Memcpy-%s] count: %-10lu, size: %lu (%s)\n",
                direction, it.second.count, it.second.size, format_size(it.second.size).c_str());
    }

    fprintf(stdout, "%-12s count: %-10lu, size: %lu (%s)\n", 
            "[TenMalloc]", ten_stats.alloc_count, ten_stats.alloc_size, format_size(ten_stats.alloc_size).c_str());
    fprintf(stdout, "%-12s count: %-10lu, size: %lu (%s)\n", 
            "[TenFree]", ten_stats.free_count, ten_stats.free_size, format_size(ten_stats.free_size).c_str());
    fprintf(stdout, "--------------------------------------------------------------------------------\n");
}
