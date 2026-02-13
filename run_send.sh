#!/bin/bash
set -e

# ================================
# Timestamp
# ================================
TS=$(date +%Y%m%d_%H%M%S)

# ================================
# 설정값
# ================================
SENDER_BIN=./build/bin/tm_sender
DST_IP=127.0.0.1
DST_PORT=5000
RAW_FILE=test_data/raw/gradient_1920x1080.raw

LOG_DIR=logs
TX_LOG=${LOG_DIR}/tx_${TS}.log

# Option
LOOP=false
SLEEP_MS=0

# ================================
# Option parsing
# ================================
while [[ $# -gt 0 ]]; do
    case "$1" in
        --loop)
            LOOP=true
            shift
            ;;
        --sleep)
            SLEEP_MS=$2
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

mkdir -p ${LOG_DIR}

echo "================================"
echo "[SEND] sender binary : ${SENDER_BIN}"
echo "[SEND] dst           : ${DST_IP}:${DST_PORT}"
echo "[SEND] raw file      : ${RAW_FILE}"
echo "[SEND] log file      : ${TX_LOG}"
echo "[SEND] loop          : ${LOOP}"
echo "================================"

# ================================
# Transmission execution
# ================================
if [ "${LOOP}" = true ]; then
    echo "[SEND] start loop sending"
    while true; do
        ${SENDER_BIN} ${DST_IP} ${DST_PORT} ${RAW_FILE} \
            | tee -a ${TX_LOG}

        if [ "${SLEEP_MS}" -gt 0 ]; then
            usleep $((SLEEP_MS * 1000))
        fi
    done
else
    echo "[SEND] send once"
    ${SENDER_BIN} ${DST_IP} ${DST_PORT} ${RAW_FILE} \
        | tee ${TX_LOG}
fi

echo "[SEND] done"
