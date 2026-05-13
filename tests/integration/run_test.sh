#!/bin/bash
# =============================================================================
# Cockpit Integration Test — vcan0 기반 전체 시스템 테스트
# 사전 조건: sudo modprobe vcan && sudo ip link add vcan0 type vcan && sudo ip link set up vcan0
# =============================================================================

set -euo pipefail

REPO="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="$REPO/build"
CAN_IF="vcan0"

RED='\033[0;31m'; GRN='\033[0;32m'; YLW='\033[0;33m'
BLU='\033[0;34m'; CYN='\033[0;36m'; RST='\033[0m'

log()  { echo -e "${CYN}[TEST]${RST} $*"; }
ok()   { echo -e "${GRN}[ OK ]${RST} $*"; }
warn() { echo -e "${YLW}[WARN]${RST} $*"; }
err()  { echo -e "${RED}[FAIL]${RST} $*"; }
sep()  { echo -e "${BLU}────────────────────────────────────────────────────────${RST}"; }

# ── 정리 핸들러 ───────────────────────────────────────────────────────────────
PIDS=()
cleanup() {
    log "종료 중..."
    for pid in "${PIDS[@]}"; do
        kill "$pid" 2>/dev/null || true
    done
    wait 2>/dev/null || true
}
trap cleanup EXIT INT TERM

# ── 전제 조건 확인 ────────────────────────────────────────────────────────────
sep
log "전제 조건 확인"

if ! ip link show "$CAN_IF" &>/dev/null; then
    err "vcan0 없음. 다음을 실행하세요:"
    err "  sudo modprobe vcan"
    err "  sudo ip link add dev vcan0 type vcan"
    err "  sudo ip link set up vcan0"
    exit 1
fi
ok "vcan0 UP"

for tool in cansend candump; do
    if ! command -v "$tool" &>/dev/null; then
        err "$tool 없음 (can-utils 설치 필요)"
        exit 1
    fi
done
ok "can-utils 설치됨"

CLUSTER_BIN="$BUILD/apps/cluster/cluster"
ENT_BIN="$BUILD/apps/entertainment/entertainment"

if [ ! -f "$CLUSTER_BIN" ]; then
    warn "cluster 바이너리 없음 → 빌드 중..."
    cmake --build "$BUILD" --target cluster -j"$(nproc)"
fi
ok "cluster 바이너리 확인"

if [ ! -f "$ENT_BIN" ]; then
    warn "entertainment 바이너리 없음 → 빌드 중..."
    cmake --build "$BUILD" --target entertainment -j"$(nproc)"
fi
ok "entertainment 바이너리 확인"

# ── 앱 실행 ───────────────────────────────────────────────────────────────────
sep
log "앱 실행"

DISPLAY=:0 CLUSTER_CAN_IF="$CAN_IF" "$CLUSTER_BIN" &
PIDS+=($!)
ok "cluster PID=${PIDS[-1]}"

DISPLAY=:0 ENTERTAINMENT_CAN_IF="$CAN_IF" "$ENT_BIN" &
PIDS+=($!)
ok "entertainment PID=${PIDS[-1]}"

sleep 1.5

# ── CAN 헬퍼 ──────────────────────────────────────────────────────────────────
# cansend vcan0 <ID>#<HEX_DATA>

# speed_rpm [speed_kph] [rpm]
send_speed_rpm() {
    local speed_x10
    speed_x10=$(( $1 * 10 ))
    local rpm=$2
    printf -v frame "%04X%04X" "$speed_x10" "$rpm"
    cansend "$CAN_IF" "400#$frame"
}

# vehicle_state [speed_kph] [rpm] [gear_byte]
#   gear_byte: 0=N, 1-6=drive, 7=R
send_vehicle_state() {
    local speed_x10
    speed_x10=$(( $1 * 10 ))
    local rpm=$2
    local gear=$3
    printf -v frame "%04X%04X%02X0000" "$speed_x10" "$rpm" "$gear"
    # 8바이트 맞춤
    frame="${frame}00"
    cansend "$CAN_IF" "500#${frame:0:16}"
}

# engine_state [coolant_c] [oil_pct] [fuel_pct]
send_engine_state() {
    printf -v frame "%02X%02X%02X" "$1" "$2" "$3"
    cansend "$CAN_IF" "501#$frame"
}

# switch_status [flags_hex]  e.g. 0x07 = IGN+ENG+LIGHT
send_switch() {
    local lo=$(( $1 & 0xFF ))
    local hi=$(( ($1 >> 8) & 0xFF ))
    printf -v frame "%02X%02X" "$lo" "$hi"
    cansend "$CAN_IF" "300#$frame"
}

# gear [0-6 | 7=R]
send_gear() {
    printf -v frame "%02X" "$1"
    cansend "$CAN_IF" "301#$frame"
}

# warning [flags_hex]
send_warning() {
    local lo=$(( $1 & 0xFF ))
    local hi=$(( ($1 >> 8) & 0xFF ))
    printf -v frame "%02X%02X" "$lo" "$hi"
    cansend "$CAN_IF" "401#$frame"
}

# position [x_m] [z_m]  (Unity 좌표 단위: meter, 정수)
send_position() {
    local xi=$(( $1 * 100 ))
    local zi=$(( $2 * 100 ))
    # 음수 처리: 32비트 부호 정수 → 16진 8자리
    printf -v xhex "%08X" $(( xi & 0xFFFFFFFF ))
    printf -v zhex "%08X" $(( zi & 0xFFFFFFFF ))
    cansend "$CAN_IF" "600#${xhex}${zhex}"
}

# heading [degrees_int]  0=North 90=East
send_heading() {
    local h=$(( $1 * 10 ))
    printf -v frame "%04X" "$h"
    cansend "$CAN_IF" "601#$frame"
}

# ── 시나리오 1: 시동 ─────────────────────────────────────────────────────────
sep
log "시나리오 1 — 시동 (Ignition ON → Engine START)"

send_switch 0x01      # Ignition ON
sleep 0.3
send_switch 0x03      # + Engine ON
send_gear 0           # N
send_engine_state 20 0 76   # 냉각수 20°C, 오일압 0%, 연료 76%
send_speed_rpm 0 800
send_vehicle_state 0 800 0
send_position 0 0
send_heading 0
ok "시동 완료 (냉각수 20°C, RPM 800, 기어 N)"
sleep 2

# ── 시나리오 2: 워밍업 ────────────────────────────────────────────────────────
sep
log "시나리오 2 — 워밍업 (냉각수 온도 상승)"

send_switch 0x07    # + HeadLight
for temp in 30 45 60 75 88; do
    send_engine_state "$temp" 75 75
    send_speed_rpm 0 900
    log "  냉각수: ${temp}°C"
    sleep 0.5
done
ok "워밍업 완료 (88°C)"
sleep 1

# ── 시나리오 3: 주행 ─────────────────────────────────────────────────────────
sep
log "시나리오 3 — 주행 (가속 → 기어 변속)"

send_gear 1
send_vehicle_state 0 1500 1
log "  1단 출발..."
sleep 0.5

px=0; pz=0
for step in $(seq 1 40); do
    speed=$(( step * 3 ))
    [ $speed -gt 120 ] && speed=120
    rpm=$(( 800 + step * 120 ))
    [ $rpm -gt 6000 ] && rpm=6000

    # 기어 자동 결정
    if   [ $speed -lt 20 ]; then gear=1
    elif [ $speed -lt 40 ]; then gear=2
    elif [ $speed -lt 60 ]; then gear=3
    elif [ $speed -lt 80 ]; then gear=4
    elif [ $speed -lt 100 ]; then gear=5
    else gear=6
    fi

    # 북쪽으로 이동 (Z 증가)
    pz=$(( pz + 3 ))

    send_speed_rpm "$speed" "$rpm"
    send_vehicle_state "$speed" "$rpm" "$gear"
    send_engine_state 88 75 $(( 76 - step / 4 ))
    send_position "$px" "$pz"
    send_heading 0

    if [ $(( step % 10 )) -eq 0 ]; then
        log "  속도: ${speed} km/h  RPM: ${rpm}  기어: ${gear}  위치: (${px}, ${pz})"
    fi
    sleep 0.1
done
ok "주행 완료 (최고 120 km/h, 6단)"
sleep 1

# ── 시나리오 4: 방향 전환 ────────────────────────────────────────────────────
sep
log "시나리오 4 — 방향 전환 (우회전 → 동쪽 주행)"

for hdg in $(seq 0 9 90); do
    send_heading "$hdg"
    sleep 0.05
done
ok "방향: 0° → 90° (East)"

for step in $(seq 1 20); do
    px=$(( px + 3 ))
    send_speed_rpm 80 3200
    send_vehicle_state 80 3200 5
    send_position "$px" "$pz"
    send_heading 90
    sleep 0.1
done
ok "동쪽 이동 완료  위치: (${px}, ${pz})"
sleep 1

# ── 시나리오 5: 경고등 ────────────────────────────────────────────────────────
sep
log "시나리오 5 — 경고등 (연료부족 + 과열)"

send_engine_state 103 75 8    # 과열 103°C, 연료 8%
send_warning 0x11             # bit0=CheckEngine, bit4=FuelLow
log "  연료부족 + 과열 경고 ON"
sleep 2

# 방향지시등 점멸
send_switch $(( 0x07 | 0x200 ))   # TurnRight bit (bit9)
log "  우측 방향지시등 ON"
sleep 2
send_switch 0x07
log "  방향지시등 OFF"
sleep 1

# ── 시나리오 6: 고속 주행 색상 변화 ──────────────────────────────────────────
sep
log "시나리오 6 — 고속 주행 (속도 색상: white→amber→red)"
send_warning 0x00
send_engine_state 88 75 40

for speed in 60 80 100 130 150; do
    rpm=$(( speed * 45 ))
    send_speed_rpm "$speed" "$rpm"
    send_vehicle_state "$speed" "$rpm" 6
    log "  속도: ${speed} km/h"
    sleep 0.8
done
sleep 1

# ── 시나리오 7: 정지 ─────────────────────────────────────────────────────────
sep
log "시나리오 7 — 감속 → 정지"

for speed in 120 90 60 30 10 0; do
    rpm=$(( 800 + speed * 30 ))
    send_speed_rpm "$speed" "$rpm"
    send_vehicle_state "$speed" "$rpm" $(( speed > 0 ? 2 : 0 ))
    sleep 0.2
done
send_gear 0
send_switch 0x01    # Engine OFF, Ignition만 유지
ok "정지 완료"

# ── 완료 ─────────────────────────────────────────────────────────────────────
sep
echo -e "${GRN}"
echo "  ✓ 통합 테스트 완료"
echo "  시나리오: 시동 → 워밍업 → 주행 → 방향전환 → 경고등 → 고속 → 정지"
echo -e "${RST}"
sep

log "앱이 열려있습니다. 확인 후 Ctrl+C 로 종료하세요."
wait
