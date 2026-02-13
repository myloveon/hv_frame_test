#!/usr/bin/env bash
set -euo pipefail


# -----------------------------
# Defaults
# -----------------------------
BUILD_DIR=build
BUILD_TYPE=Debug        # Default is Debug
ENABLE_DEBUG_LOG=OFF    # Default is OFF

# -----------------------------
# Parse arguments
# -----------------------------
for arg in "$@"; do
    case $arg in
        release|Release)
            BUILD_TYPE=Release
            ;;
        debuglog)
            ENABLE_DEBUG_LOG=ON
            ;;
        *)
            echo "Usage:"
            echo "  ./build.sh              # Debug build"
            echo "  ./build.sh debuglog     # Debug + debug_log"
            echo "  ./build.sh release      # Release build"
            exit 1
            ;;
    esac
done

# -----------------------------
# Enforce rules
# -----------------------------
if [ "$BUILD_TYPE" = "Release" ]; then
    ENABLE_DEBUG_LOG=OFF
fi

echo "===================================="
echo " Build type        : $BUILD_TYPE"
echo " Debug log enabled : $ENABLE_DEBUG_LOG"
echo "===================================="

# -----------------------------
# Configure
# -----------------------------
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=${BUILD_TYPE}"

if [ "$ENABLE_DEBUG_LOG" = "ON" ]; then
    CMAKE_ARGS="${CMAKE_ARGS} -DDEBUG_LOG_ENABLE=ON"
fi

rm -rf build
cmake -S . -B build ${CMAKE_ARGS}

# -----------------------------
# Build
# -----------------------------
cmake --build build -j$(nproc)

echo "Build finished successfully. Binaries (if built): $BUILD_DIR/bin/tm_sender $BUILD_DIR/bin/tm_receiver"




#./build.sh
#=> Debug, debug_log OFF

#./build.sh debuglog
#=> Debug, debug_log ON

#./build.sh release
#=> Release, debug_log OFF (Forced)

#./build.sh release debuglog
#=> Release, debug_log OFF (Disregarded)

