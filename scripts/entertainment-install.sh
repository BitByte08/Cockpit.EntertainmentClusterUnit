#!/bin/bash
# =============================================================================
# Entertainment Unit — Raspberry Pi 설치 스크립트
# Seengreat RS485 Dual CAN I HAT 기반
# 참고: https://seengreat.com/wiki/83/rs485-dual-can-i
#
# 사용법:
#   sudo bash entertainment-install.sh [바이너리경로]
#   예) sudo bash entertainment-install.sh ./entertainment-arm64
# =============================================================================
set -euo pipefail

INSTALL_DIR="/opt/entertainment"
BINARY_SRC="${1:-./entertainment-arm64}"
GITHUB_REPO="BitByte08/Cockpit.EntertainmentClusterUnit"
RELEASE_BASE="https://github.com/${GITHUB_REPO}/releases/latest/download"
PI_USER="${SUDO_USER:-pi}"
TILES_DIR="${INSTALL_DIR}/tiles"

RED='\033[0;31m'; GRN='\033[0;32m'; YLW='\033[1;33m'; NC='\033[0m'
info()    { echo -e "${GRN}[INFO]${NC}  $*"; }
warn()    { echo -e "${YLW}[WARN]${NC}  $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*" >&2; }
section() { echo -e "\n${GRN}══ $* ══${NC}"; }

if [[ $EUID -ne 0 ]]; then
    error "루트 권한 필요: sudo bash entertainment-install.sh"
    exit 1
fi

BOOT_CFG="/boot/firmware/config.txt"
[[ -f "$BOOT_CFG" ]] || BOOT_CFG="/boot/config.txt"
info "부트 설정 파일: $BOOT_CFG"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

download_release() {
    local filename="$1" dest="$2"
    local base
    base="$(basename "$filename")"
    local candidates=("$filename" "$base" "scripts/$base")

    for dir in "$SCRIPT_DIR" "$(dirname "$BINARY_SRC")"; do
        for candidate in "${candidates[@]}"; do
            [[ -f "${dir}/${candidate}" ]] && { cp "${dir}/${candidate}" "$dest"; return 0; }
        done
    done

    local tmp="${dest}.download"
    if curl -fsSL "${RELEASE_BASE}/${base}" -o "$tmp"; then
        mv "$tmp" "$dest"
        return 0
    fi
    rm -f "$tmp"
    return 1
}

# ── 1. 시스템 패키지 설치 ────────────────────────────────────────────────────
section "1/7 패키지 설치"
apt-get update -q
apt-get install -y --no-install-recommends \
    can-utils \
    iproute2 \
    curl \
    python3 \
    xxd \
    libqt6widgets6 \
    libqt6core6 \
    libqt6gui6 \
    xserver-xorg-core \
    xinit \
    x11-utils \
    x11-xserver-utils \
    fonts-noto-cjk \
    unclutter \
    openbox
info "패키지 설치 완료"

# ── 2. SPI 활성화 ────────────────────────────────────────────────────────────
section "2/7 SPI 인터페이스 활성화"
if ! grep -q "^dtparam=spi=on" "$BOOT_CFG"; then
    echo "dtparam=spi=on" >> "$BOOT_CFG"
    info "SPI 활성화 추가"
else
    info "SPI 이미 활성화됨"
fi

# ── 3. CAN HAT dtoverlay 설정 ─────────────────────────────────────────────────
section "3/7 CAN HAT dtoverlay 설정"
OVERLAYS=(
    "dtoverlay=spi1-3cs"
    "dtoverlay=mcp2515,spi0-1,oscillator=16000000,interrupt=25"
    "dtoverlay=mcp2515,spi1-1,oscillator=16000000,interrupt=24"
    "dtoverlay=uart2"
)
for OVERLAY in "${OVERLAYS[@]}"; do
    if ! grep -qF "$OVERLAY" "$BOOT_CFG"; then
        echo "$OVERLAY" >> "$BOOT_CFG"
        info "추가: $OVERLAY"
    else
        info "이미 존재: $OVERLAY"
    fi
done

# ── 4. CAN 인터페이스 서비스 ─────────────────────────────────────────────────
section "4/7 CAN 인터페이스 서비스 설치"
cat > /etc/systemd/system/can-setup.service << 'EOF'
[Unit]
Description=CAN Bus Interface Setup
After=network.target
Before=entertainment-kiosk.service

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStart=/bin/bash -c "\
    /sbin/ip link set can0 up type can bitrate 500000 2>/dev/null || true; \
    /sbin/ip link set can1 up type can bitrate 500000 2>/dev/null || true"
ExecStop=/bin/bash -c "\
    /sbin/ip link set can0 down 2>/dev/null || true; \
    /sbin/ip link set can1 down 2>/dev/null || true"

[Install]
WantedBy=multi-user.target
EOF
info "can-setup.service 생성 완료"

# ── 5. 앱 설치 ───────────────────────────────────────────────────────────────
section "5/7 엔터테인먼트 앱 설치"
mkdir -p "$INSTALL_DIR" "$TILES_DIR"

systemctl stop entertainment-kiosk.service 2>/dev/null || true
pkill -9 -f "/opt/entertainment/entertainment" 2>/dev/null || true
sleep 1

if [[ -f "$BINARY_SRC" ]]; then
    cp "$BINARY_SRC" "${INSTALL_DIR}/entertainment"
    chmod +x "${INSTALL_DIR}/entertainment"
    info "바이너리 설치: ${INSTALL_DIR}/entertainment"
else
    warn "바이너리를 찾을 수 없음 ($BINARY_SRC) — 나중에 OTA로 받습니다"
    touch "${INSTALL_DIR}/entertainment"
    chmod +x "${INSTALL_DIR}/entertainment"
fi

# update.sh
if download_release entertainment-update.sh "${INSTALL_DIR}/update.sh"; then
    chmod +x "${INSTALL_DIR}/update.sh"
    info "update.sh 설치 완료"
else
    warn "update.sh를 찾을 수 없음"
fi

# VERSION
CURRENT_VERSION="0.0.0"
if download_release VERSION "${INSTALL_DIR}/VERSION"; then
    CURRENT_VERSION=$(tr -d '[:space:]' < "${INSTALL_DIR}/VERSION")
else
    echo "$CURRENT_VERSION" > "${INSTALL_DIR}/VERSION"
fi
info "VERSION: $CURRENT_VERSION"

# 맵 타일 설치 (map-assets.tar.gz)
if [[ -z "$(ls -A "$TILES_DIR" 2>/dev/null)" ]]; then
    info "맵 타일 다운로드 중..."
    MAP_PKG_TMP="${INSTALL_DIR}/map-assets.tar.gz"
    if download_release map-assets.tar.gz "$MAP_PKG_TMP"; then
        tar -xzf "$MAP_PKG_TMP" -C "$INSTALL_DIR"
        rm -f "$MAP_PKG_TMP"
        TILE_COUNT=$(find "$TILES_DIR" -name "*.png" | wc -l)
        info "타일 설치 완료: ${TILE_COUNT}개 → $TILES_DIR"
    else
        warn "map-assets.tar.gz 다운로드 실패 — 수동으로 복사하세요"
        warn "  scp map-assets.tar.gz pi@<IP>:${INSTALL_DIR}/"
        warn "  ssh pi@<IP> 'sudo tar -xzf ${INSTALL_DIR}/map-assets.tar.gz -C ${INSTALL_DIR}/'"
    fi
else
    TILE_COUNT=$(find "$TILES_DIR" -name "*.png" | wc -l)
    info "타일 이미 설치됨: ${TILE_COUNT}개"
fi

# road_graph.json (맵 패키지에 포함됐으면 이미 있고, 없으면 별도 다운로드)
if [[ ! -f "${INSTALL_DIR}/road_graph.json" ]]; then
    if download_release road_graph.json "${INSTALL_DIR}/road_graph.json"; then
        info "road_graph.json 다운로드 완료"
    else
        warn "road_graph.json 없음 — 네비게이션 지도 비활성화"
    fi
else
    info "road_graph.json 이미 설치됨"
fi

# ── 6. systemd 서비스 등록 ───────────────────────────────────────────────────
section "6/7 systemd 서비스 등록"

# entertainment-update.service
if ! download_release entertainment-update.service \
        /etc/systemd/system/entertainment-update.service; then
    cat > /etc/systemd/system/entertainment-update.service << EOF
[Unit]
Description=Car Entertainment OTA Update Check
After=network-online.target
Wants=network-online.target
Before=entertainment-kiosk.service

[Service]
Type=oneshot
ExecStart=${INSTALL_DIR}/update.sh
RemainAfterExit=yes
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF
fi
info "entertainment-update.service 설치 완료"

# entertainment-kiosk.service (사용자 치환)
KIOSK_TMP=$(mktemp)
if download_release entertainment-kiosk.service "$KIOSK_TMP"; then
    sed "s/User=pi/User=${PI_USER}/g; s/Group=pi/Group=${PI_USER}/g; \
         s|/home/pi/|/home/${PI_USER}/|g" \
        "$KIOSK_TMP" > /etc/systemd/system/entertainment-kiosk.service
    rm -f "$KIOSK_TMP"
else
    rm -f "$KIOSK_TMP"
    cat > /etc/systemd/system/entertainment-kiosk.service << EOF
[Unit]
Description=Car Entertainment Kiosk Application
After=entertainment-update.service can-setup.service graphical.target
Wants=entertainment-update.service can-setup.service

[Service]
Type=simple
User=${PI_USER}
Group=${PI_USER}
WorkingDirectory=${INSTALL_DIR}
Environment=DISPLAY=:0
Environment=XAUTHORITY=/home/${PI_USER}/.Xauthority
Environment=QT_QPA_PLATFORM=xcb
Environment=QT_QPA_PLATFORMTHEME=
Environment=XDG_RUNTIME_DIR=/run/user/1000
Environment=ENTERTAINMENT_KIOSK=1
Environment=ENTERTAINMENT_CAN_IF=can0
ExecStartPre=/bin/bash -c "until xdpyinfo -display :0 >/dev/null 2>&1; do sleep 0.5; done"
ExecStart=${INSTALL_DIR}/entertainment
Restart=always
RestartSec=3
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=graphical.target
EOF
fi
info "entertainment-kiosk.service 설치 완료"

# X 서버 자동 시작 (.xinitrc)
XINIT_FILE="/home/${PI_USER}/.xinitrc"
cat > "$XINIT_FILE" << 'XINITEOF'
#!/bin/bash
xset s off
xset s noblank
xset -dpms
xsetroot -cursor_name none &
unclutter -idle 0 -root &
openbox &
while true; do
    ENTERTAINMENT_KIOSK=1 ENTERTAINMENT_CAN_IF=can0 \
        /opt/entertainment/entertainment
    sleep 1
done
XINITEOF
chown "${PI_USER}:${PI_USER}" "$XINIT_FILE"
chmod +x "$XINIT_FILE"

# 자동 로그인 후 X 시작 (.bash_profile)
PROFILE_FILE="/home/${PI_USER}/.bash_profile"
if ! grep -q "startx" "$PROFILE_FILE" 2>/dev/null; then
    cat >> "$PROFILE_FILE" << 'PROFILEEOF'

# 엔터테인먼트 키오스크 자동 시작
if [[ -z "$DISPLAY" && "$(tty)" == "/dev/tty1" ]]; then
    exec startx
fi
PROFILEEOF
    info ".bash_profile에 startx 자동 시작 추가"
fi

# 콘솔 자동 로그인
systemctl set-default multi-user.target
mkdir -p /etc/systemd/system/getty@tty1.service.d
cat > /etc/systemd/system/getty@tty1.service.d/autologin.conf << EOF
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin ${PI_USER} --noclear %I \$TERM
EOF
info "tty1 자동 로그인 설정 완료 (${PI_USER})"

systemctl daemon-reload
systemctl enable can-setup.service             || warn "can-setup.service enable 실패"
systemctl enable entertainment-update.service  || warn "entertainment-update.service enable 실패"
systemctl enable entertainment-kiosk.service   || warn "entertainment-kiosk.service enable 실패"
info "서비스 활성화 완료"

# ── 7. RS485 직렬 포트 설정 ──────────────────────────────────────────────────
section "7/7 RS485 직렬 포트 설정"
if systemctl is-active --quiet serial-getty@ttyS0.service 2>/dev/null; then
    systemctl stop serial-getty@ttyS0.service
    systemctl disable serial-getty@ttyS0.service
    info "serial-getty@ttyS0 비활성화 완료"
fi

CMDLINE_FILE="/boot/firmware/cmdline.txt"
[[ -f "$CMDLINE_FILE" ]] || CMDLINE_FILE="/boot/cmdline.txt"
if grep -q "console=serial0" "$CMDLINE_FILE" 2>/dev/null; then
    sed -i 's/console=serial0,[0-9]* //g' "$CMDLINE_FILE"
    info "cmdline.txt에서 console=serial0 제거"
fi

# ── 완료 ─────────────────────────────────────────────────────────────────────
echo ""
echo -e "${GRN}╔══════════════════════════════════════════════╗${NC}"
echo -e "${GRN}║   Entertainment Unit 설치 완료! 재부팅 필요  ║${NC}"
echo -e "${GRN}╚══════════════════════════════════════════════╝${NC}"
echo ""
info "CAN 인터페이스: can0 (SPI0, INT=BCM25), can1 (SPI1, INT=BCM24)"
info "CAN 비트레이트: 500Kbps"
info "설치 경로: ${INSTALL_DIR}"
info "타일 경로: ${TILES_DIR}/{zoom}/{x}/{y}.png"
warn "3.3V/5V 로직 전압 슬라이드 스위치를 3.3V로 설정하세요!"
echo ""
echo "타일 복사 방법 (다른 PC에서):"
echo "  scp -r tiles/ ${PI_USER}@<PI_IP>:${TILES_DIR}/"
echo ""
echo "재부팅: sudo reboot"
