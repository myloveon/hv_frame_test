cmake_minimum_required(VERSION 3.10)
project(MCSE_UDP_TM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# POSIX feature test macro and common warnings
add_definitions(-D_POSIX_C_SOURCE=200112L)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Output directory for binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Sender sources
set(SENDER_SRCS
    src/sender/main_sender.cpp
    src/sender/file_source.cpp
    src/sender/ccsds_stub_encoder.cpp
    src/sender/udp_sender.cpp
)

# Receiver sources
set(RECEIVER_SRCS
    src/receiver/main_receiver.cpp
    src/common/frame_reassembler.cpp
)

add_executable(tm_sender ${SENDER_SRCS})
target_compile_options(tm_sender PRIVATE -O3 -Wall)
target_link_libraries(tm_sender PRIVATE pthread)

add_executable(tm_receiver ${RECEIVER_SRCS})

# Platform-specific options for receiver
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    message(STATUS "Configuring for AArch64 (AGX Orin)")
    target_compile_options(tm_receiver PRIVATE -O3 -march=armv8.2-a+crypto -Wall)
else()
    message(STATUS "Configuring for Local x86_64 Host")
    target_compile_options(tm_receiver PRIVATE -O3 -Wall)
endif()

target_link_libraries(tm_receiver PRIVATE pthread)
