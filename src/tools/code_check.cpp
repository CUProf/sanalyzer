
#include "tools/code_check.h"
#include "utils/helper.h"
#include "gpu_patch.h"
#include "cxx_backtrace.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <iostream>


using namespace yosemite;



static Timer_t _timer;


static std::map<uint64_t, std::shared_ptr<KernelLauch_t>> kernel_events;
static std::map<uint64_t, std::shared_ptr<MemAlloc_t>> alloc_events;
static std::map<DevPtr, std::shared_ptr<MemAlloc_t>> active_memories;


void CodeCheck::init() {
    const char* env_name = std::getenv("CU_PROF_HOME");
    std::string lib_path;
    if (env_name) {
        lib_path = std::string(env_name) + "/lib/libcompute_sanitizer.so";
    }
    init_back_trace(lib_path.c_str());

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
        default:
            break;
    }
}


void CodeCheck::kernel_start_callback(std::shared_ptr<KernelLauch_t> kernel) {

    _timer.increment(true);
}


void CodeCheck::kernel_end_callback(std::shared_ptr<KernelEnd_t> kernel) {
}


void CodeCheck::mem_alloc_callback(std::shared_ptr<MemAlloc_t> mem) {
    

    _timer.increment(true);
}


void CodeCheck::mem_free_callback(std::shared_ptr<MemFree_t> mem) {
   

    _timer.increment(true);
}


void CodeCheck::mem_cpy_callback(std::shared_ptr<MemCpy_t> mem) {
    
    std::cout << "Memory copy detected" << std::endl;
    std::cout << mem->src_addr << " " << mem->dst_addr << " " << mem->size << std::endl;
    std::cout << "Is async: " << mem->is_async << std::endl;
    std::cout << "Direction: " << mem->direction << std::endl;

    std::cout << get_back_trace() << std::endl;

    _timer.increment(true);
}


void CodeCheck::mem_set_callback(std::shared_ptr<MemSet_t> mem) {
    

    _timer.increment(true);
}


void CodeCheck::gpu_data_analysis(void* data, size_t size) {
    
}


void CodeCheck::flush() {
    
}