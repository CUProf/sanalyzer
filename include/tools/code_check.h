#ifndef YOSEMITE_TOOL_CODE_CHECK_H
#define YOSEMITE_TOOL_CODE_CHECK_H


#include "tools/tool.h"
#include "utils/event.h"

namespace yosemite {

class CodeCheck final : public Tool {
public:
    CodeCheck() : Tool(CODE_CHECK) {
        init();
    }

    void init();

    ~CodeCheck() {}

    void kernel_start_callback(std::shared_ptr<KernelLauch_t> kernel);

    void kernel_end_callback(std::shared_ptr<KernelEnd_t> kernel);

    void mem_alloc_callback(std::shared_ptr<MemAlloc_t> mem);

    void mem_free_callback(std::shared_ptr<MemFree_t> mem);

    void mem_cpy_callback(std::shared_ptr<MemCpy_t> mem);

    void mem_set_callback(std::shared_ptr<MemSet_t> mem);

    void ten_alloc_callback(std::shared_ptr<TenAlloc_t> ten);

    void ten_free_callback(std::shared_ptr<TenFree_t> ten);

    void evt_callback(EventPtr_t evt);

    void gpu_data_analysis(void* data, uint64_t size);

    void query_ranges(void* ranges, uint32_t limit, uint32_t* count);

    void flush();
};

}   // yosemite
#endif // YOSEMITE_TOOL_CODE_CHECK_H