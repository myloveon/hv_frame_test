#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR=build

echo "Configure: cmake -S . -B $BUILD_DIR"
cmake -S . -B "$BUILD_DIR"

echo "Build: cmake --build $BUILD_DIR -j\$(nproc)"
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "Build finished. Binaries (if built): $BUILD_DIR/bin/tm_sender $BUILD_DIR/bin/tm_receiver"
