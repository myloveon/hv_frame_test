#!/usr/bin/env bash
set -euo pipefail

# Simple end-to-end test: build, start receiver, send one frame, verify outputs
# Usage: ./run_test.sh [--no-stop-receiver] [--no-keep-log] [--port N] [--raw path]

# ./run_test.sh

#./run_test.sh --no-stop-receiver

#./run_test.sh --no-keep-log

#./run_test.sh --port 6000 --raw test_data/raw/my.raw

BUILD_SCRIPT="./build.sh"
RECEIVER_PORT=5000
RAW_PATH="test_data/raw/gradient_1920x1080.raw"
RECEIVER_LOG="tm_receiver_debug.log"

# options
STOP_RECEIVER=true
KEEP_LOG=true

while [ "$#" -gt 0 ]; do
  case "$1" in
    --no-stop-receiver|--no-stop)
      STOP_RECEIVER=false; shift ;;
    --no-keep-log|--no-preserve-log)
      KEEP_LOG=false; shift ;;
    --port|-p)
      shift; RECEIVER_PORT="$1"; shift ;;
    --raw|-r)
      shift; RAW_PATH="$1"; shift ;;
    --help|-h)
      echo "Usage: $0 [--no-stop-receiver] [--no-keep-log] [--port N] [--raw path]"; exit 0;;
    *) echo "Unknown option: $1"; exit 1;;
  esac
done

echo "1) Build"
"$BUILD_SCRIPT"

echo "2) Stop any existing receiver"
pkill -f tm_receiver || true
sleep 0.1

echo "3) Start receiver (background)"
nohup ./build/bin/tm_receiver "$RECEIVER_PORT" > "$RECEIVER_LOG" 2>&1 &
RECV_PID=$!
echo "receiver pid=$RECV_PID (log=$RECEIVER_LOG)"
sleep 0.2

echo "4) Send frame"
./build/bin/tm_sender 127.0.0.1 "$RECEIVER_PORT" "$RAW_PATH"

echo "5) Wait briefly for reassembly"
sleep 0.5

echo "6) List generated files"
ls -l sender_header.bin received_header.bin received_raw.bin received_frame.bin || true

if [ -f sender_header.bin ] && [ -f received_header.bin ]; then
  echo "Header checksums:"
  sha256sum sender_header.bin received_header.bin || true
fi

if [ -f received_raw.bin ] && [ -f "$RAW_PATH" ]; then
  if cmp --silent received_raw.bin "$RAW_PATH"; then
    echo "received_raw.bin IDENTICAL to $RAW_PATH"
  else
    echo "received_raw.bin DIFFERENT from $RAW_PATH"
  fi
fi

echo "Last lines of receiver log:"
tail -n 40 "$RECEIVER_LOG" || true

if [ "$STOP_RECEIVER" = true ]; then
  echo "Stopping receiver pid=$RECV_PID"
  pkill -P "$RECV_PID" || true
  kill "$RECV_PID" 2>/dev/null || true
  wait "$RECV_PID" 2>/dev/null || true
fi

if [ "$KEEP_LOG" = true ]; then
  TS=$(date +%Y%m%d_%H%M%S)
  mv "$RECEIVER_LOG" "${RECEIVER_LOG%.log}_$TS.log" || true
  echo "Receiver log preserved as ${RECEIVER_LOG%.log}_$TS.log"
else
  rm -f "$RECEIVER_LOG" || true
  echo "Receiver log removed"
fi

echo "Done."
