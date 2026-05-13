#!/bin/bash

# CAN smoke test for the current Cluster/Entertainment protocol.
# Usage: ./test_can.sh [can-interface]

set -euo pipefail

CAN_IF="${1:-vcan0}"

echo "=== CAN smoke test ==="
echo "Interface: ${CAN_IF}"
echo ""

if ! ip link show "$CAN_IF" >/dev/null 2>&1; then
    echo "[ERROR] ${CAN_IF} not found"
    echo "For local testing:"
    echo "  sudo modprobe vcan"
    echo "  sudo ip link add dev vcan0 type vcan"
    echo "  sudo ip link set up vcan0"
    exit 1
fi

if ! command -v cansend >/dev/null 2>&1; then
    echo "[ERROR] cansend not found. Install can-utils."
    exit 1
fi

send_switch() {
    local flags=$1
    local lo=$(( flags & 0xFF ))
    local hi=$(( (flags >> 8) & 0xFF ))
    local frame
    printf -v frame "%02X%02X" "$lo" "$hi"
    cansend "$CAN_IF" "300#$frame"
}

send_gear() {
    local gear=$1
    local frame
    printf -v frame "%02X" "$gear"
    cansend "$CAN_IF" "301#$frame"
}

send_speed_rpm() {
    local speed_x10=$(( $1 * 10 ))
    local rpm=$2
    local frame
    printf -v frame "%04X%04X" "$speed_x10" "$rpm"
    cansend "$CAN_IF" "400#$frame"
}

send_warning() {
    local flags=$1
    local lo=$(( flags & 0xFF ))
    local hi=$(( (flags >> 8) & 0xFF ))
    local frame
    printf -v frame "%02X%02X" "$lo" "$hi"
    cansend "$CAN_IF" "401#$frame"
}

send_vehicle_state() {
    local speed_x10=$(( $1 * 10 ))
    local rpm=$2
    local gear=$3
    local flags=$4
    local frame
    printf -v frame "%04X%04X%02X%02X" "$speed_x10" "$rpm" "$gear" "$flags"
    cansend "$CAN_IF" "500#$frame"
}

send_engine_state() {
    local coolant=$1
    local oil=$2
    local fuel=$3
    local frame
    printf -v frame "%02X%02X%02X" "$coolant" "$oil" "$fuel"
    cansend "$CAN_IF" "501#$frame"
}

send_position() {
    local xi=$(( $1 * 100 ))
    local zi=$(( $2 * 100 ))
    local xhex zhex
    printf -v xhex "%08X" $(( xi & 0xFFFFFFFF ))
    printf -v zhex "%08X" $(( zi & 0xFFFFFFFF ))
    cansend "$CAN_IF" "600#${xhex}${zhex}"
}

send_heading() {
    local heading_x10=$(( $1 * 10 ))
    local frame
    printf -v frame "%04X" "$heading_x10"
    cansend "$CAN_IF" "601#$frame"
}

echo "Ignition + engine ON"
send_switch 0x03
send_gear 0
send_engine_state 88 60 80
send_speed_rpm 0 900
send_vehicle_state 0 900 0 0
send_position 0 0
send_heading 0
sleep 0.5

echo "Accelerating 0 -> 120 km/h"
for speed in {0..120..10}; do
    rpm=$(( 900 + speed * 45 ))
    gear=1
    if   (( speed >= 100 )); then gear=6
    elif (( speed >= 80 ));  then gear=5
    elif (( speed >= 60 ));  then gear=4
    elif (( speed >= 40 ));  then gear=3
    elif (( speed >= 20 ));  then gear=2
    fi

    send_speed_rpm "$speed" "$rpm"
    send_vehicle_state "$speed" "$rpm" "$gear" 0
    send_engine_state 90 60 $(( 80 - speed / 10 ))
    send_position 0 "$speed"
    send_heading 0
    printf "  speed=%3d km/h rpm=%4d gear=%d\n" "$speed" "$rpm" "$gear"
    sleep 0.15
done

echo "Turn right + ABS/TCS active"
send_switch $(( 0x03 | 0x200 ))
send_vehicle_state 80 3600 5 0x03
sleep 1

echo "Warning: check engine + fuel low"
send_warning 0x11
send_engine_state 103 45 8
sleep 1

echo "Clear warnings and stop"
send_warning 0
send_switch 0x01
send_speed_rpm 0 800
send_vehicle_state 0 800 0 0
send_gear 0

echo ""
echo "Individual examples:"
echo "  cansend ${CAN_IF} 400#03E80BB8        # 100.0 km/h, 3000 rpm"
echo "  cansend ${CAN_IF} 501#5F3C48          # coolant 95 C, oil 60%, fuel 72%"
echo "  cansend ${CAN_IF} 600#000004D2FFFFFE0C # x=12.34 m, z=-5.00 m"
echo ""
echo "=== Done ==="
