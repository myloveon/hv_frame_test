/*-------------------------------------------*/
/*                                           */
/* frame_writer.cpp                          */     
/*                                          */
/* Frame file writer implementation         */
/*                                          */
/* Author: hive technologies Co., Ltd.      */
/* Created on: 2026-01-10                   */ 
/* ------------------------------------------*/

#include <cstring>
#include <iostream>

#include "common/frame_writer.hpp"
#include "debug/hv_debug.hpp"
#include "protocol/frame_header.hpp"


/* ================================
 * Frame file writer
 * ================================ */

void write_frame_to_file(const uint8_t* frame,
                            size_t frame_size,
                            bool is_partial)
{
    if (!frame || frame_size < sizeof(FrameHeader))
    {
        HV_LOGE(hv::debug::Module::FRAME,
                "write_frame_to_file_v2: EMPTY frame");
        return;
    }

    const char* base = is_partial ? "partial" : "full";

    fprintf(stderr,
            "WRITE_FRAME size=%zu partial=%d\n",
            frame_size, is_partial);
    fflush(stderr);

    // full frame
    {
        std::string name =
            std::string("received_") + base + "_frame.bin";

        FILE* fp = std::fopen(name.c_str(), "wb");
        if (fp)
        {
            std::fwrite(frame, 1, frame_size, fp);
            std::fclose(fp);
        }
    }

    // header
    {
        std::string name =
            std::string("received_") + base + "_header.bin";

        FILE* fp = std::fopen(name.c_str(), "wb");
        if (fp)
        {
            std::fwrite(frame, 1, sizeof(FrameHeader), fp);
            std::fclose(fp);
        }
    }

    // raw payload
    {
        std::string name =
            std::string("received_") + base + "_raw.bin";

        FILE* fp = std::fopen(name.c_str(), "wb");
        if (fp)
        {
            std::fwrite(frame + sizeof(FrameHeader),
                         1,
                         frame_size - sizeof(FrameHeader),
                         fp);
            std::fclose(fp);
        }
    }

    HV_LOGI(hv::debug::Module::FRAME,
            "Frame written (%s): total=%zu",
            base, frame_size);
}

