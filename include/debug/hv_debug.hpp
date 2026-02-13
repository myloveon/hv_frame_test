/*================================================================================*/
    /* HV Debug Module Interface */
/*================================================================================*/



#pragma once
#include "hv_debug_cfg.hpp"

namespace hv::debug {

/* lifecycle */
void init();

/* configuration */
void setLevel(Level level);
void enable(uint32_t module_mask);
void disable(uint32_t module_mask);

/* core log */
void log(Level level, uint32_t module, const char* fmt, ...);

} // namespace hv::debug

/* Convenience macros (kept for call-site simplicity) */
#define HV_LOGE(mod, fmt, ...) \
    hv::debug::log(hv::debug::Level::Error, mod, fmt, ##__VA_ARGS__)

#define HV_LOGW(mod, fmt, ...) \
    hv::debug::log(hv::debug::Level::Warn, mod, fmt, ##__VA_ARGS__)

#define HV_LOGI(mod, fmt, ...) \
    hv::debug::log(hv::debug::Level::Info, mod, fmt, ##__VA_ARGS__)

#define HV_LOGD(mod, fmt, ...) \
    hv::debug::log(hv::debug::Level::Debug, mod, fmt, ##__VA_ARGS__)
