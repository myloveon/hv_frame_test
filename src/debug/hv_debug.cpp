/*========================================================================================*/
/*  HV Debug Module                                                                     */

/*  Created on: 2024. 6. 20.	                                                        */
/*  Debug module implementation                                                        */
/*========================================================================================*/



#include "debug/hv_debug.hpp"

#include <atomic>
#include <cstdio>
#include <cstdarg>

namespace hv::debug {

static std::atomic<uint32_t> g_module_mask{Module::All};
static std::atomic<Level>    g_level{Level::Info};

void init()
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    //setvbuf(stderr, nullptr, _IONBF, 0);
}

void setLevel(Level level)
{
    g_level.store(level, std::memory_order_relaxed);
}

void enable(uint32_t mask)
{
    g_module_mask.fetch_or(mask, std::memory_order_relaxed);
}

void disable(uint32_t mask)
{
    g_module_mask.fetch_and(~mask, std::memory_order_relaxed);
}

void log(Level level, uint32_t module, const char* fmt, ...)
{

    /* Hot Pass Protection: Return immediately if conditions are not met */
   if (static_cast<uint8_t>(level) >
        static_cast<uint8_t>(g_level.load(std::memory_order_relaxed)))
        return;
    
    /* Actual output only when reaching this point */
    if ((g_module_mask.load() & module) == 0)
        return;

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    std::printf("\n");
    va_end(args);
}

} // namespace hv::debug

