/*========================================================================================*/
/* hv_debug_cfg.hpp
 *
 *  Created on: 2024. 6. 20.	
 * 
 *  Debug configuration header
 * 
 * 	- Log level enumration
 *  - Debug module bitmask enumeration
 *========================================================================================*/

#pragma once
#include <cstdint>

namespace hv::debug {

/* Log level (ordered) */
enum class Level : uint8_t {
    Error = 0,			// Critical error message
    Warn,				// abnormal signs
    Info,				// Status information
    Debug				// Developer details
};

/* Debug Module bitmask */
enum Module : uint32_t {
    RX    = 1u << 0,
    TX    = 1u << 1,
    FRAME = 1u << 2,
    QUEUE = 1u << 3,
    All   = 0xFFFFFFFFu
};

} // namespace hv::debug
