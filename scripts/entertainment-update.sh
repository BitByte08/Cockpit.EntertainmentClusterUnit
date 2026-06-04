#!/bin/bash
# Entertainment Unit OTA Update Script
# 부팅 시 GitHub Releases에서 최신 버전 확인 후 업데이트
# API 호출 없이 releases/latest/download 직접 다운로드 (rate limit 우회)
set -euo pipefail

INSTALL_DIR="/opt/entertainment"
BINARY_NAME="entertainment"
SERVICE_NAME="entertainment-kiosk.service"
ASSET_NAME="entertainment-arm64"
GITHUB_REPO="BitByte08/Cockpit.EntertainmentClusterUnit"
DOWNLOAD_BASE="https://github.com/${GITHUB_REPO}/releases/latest/download"
VERSION_FILE="${INSTALL_DIR}/VERSION"
# 같은 파일시스템에 받아야 rename(2)로 atomic 교체 가능 (ETXTBSY 회피)
TMP_BINARY="${INSTALL_DIR}/.${BINARY_NAME}.new"
TMP_VERSION="/tmp/entertainment-latest-version"
LOG_TAG="entertainment-update"

log() { logger -t "$LOG_TAG" "$*"; echo "[$(date '+%H:%M:%S')] $*"; }

CURRENT_VERSION="0.0.0"
if [[ -f "$VERSION_FILE" ]]; then
    CURRENT_VERSION=$(tr -d '[:space:]' < "$VERSION_FILE")
fi
log "현재 버전: v${CURRENT_VERSION}"

rm -f "$TMP_VERSION"
if ! curl -sfL --max-time 10 -H "User-Agent: entertainment-update" \
    "${DOWNLOAD_BASE}/VERSION" -o "$TMP_VERSION"; then
    log "릴리스 조회 실패"
    rm -f "$TMP_VERSION"
    exit 0
fi
LATEST_VERSION=$(tr -d '[:space:]' < "$TMP_VERSION")
rm -f "$TMP_VERSION"

if [[ -z "$LATEST_VERSION" ]]; then
    log "릴리스 정보 파싱 실패"; exit 0
fi
log "최신 버전: v${LATEST_VERSION}"

version_gt() {
    local IFS='.'
    local -a a=($1) b=($2)
    for i in 0 1 2; do
        local av=${a[$i]:-0} bv=${b[$i]:-0}
        ((av > bv)) && return 0
        ((av < bv)) && return 1
    done
    return 1
}

if ! version_gt "$LATEST_VERSION" "$CURRENT_VERSION"; then
    log "이미 최신 버전 — 업데이트 불필요"; exit 0
fi
log "업데이트 발견: v${CURRENT_VERSION} → v${LATEST_VERSION}"

log "다운로드 중: ${DOWNLOAD_BASE}/${ASSET_NAME}"
mkdir -p "$INSTALL_DIR"
rm -f "$TMP_BINARY"
if ! curl -fL --max-time 120 --progress-bar -H "User-Agent: entertainment-update" \
    "${DOWNLOAD_BASE}/${ASSET_NAME}" -o "$TMP_BINARY"; then
    log "다운로드 실패"; rm -f "$TMP_BINARY"; exit 0
fi

MAGIC=$(xxd -l 4 "$TMP_BINARY" 2>/dev/null | awk '{print $2$3}')
if [[ "$MAGIC" != "7f454c46" ]]; then
    log "유효한 ELF 바이너리가 아님"; rm -f "$TMP_BINARY"; exit 0
fi

chmod 755 "$TMP_BINARY"
chown root:root "$TMP_BINARY" 2>/dev/null || true

systemctl stop "$SERVICE_NAME" 2>/dev/null || true

# 같은 파일시스템(${INSTALL_DIR}) 내 mv → rename(2) → 실행 중 바이너리도 atomic 교체
if ! mv -f "$TMP_BINARY" "${INSTALL_DIR}/${BINARY_NAME}"; then
    log "교체 실패"; rm -f "$TMP_BINARY"; exit 1
fi

echo "$LATEST_VERSION" > "$VERSION_FILE"

if ! systemctl is-system-running 2>/dev/null | grep -q 'booting'; then
    systemctl start "$SERVICE_NAME" 2>/dev/null || true
fi

log "업데이트 완료: v${LATEST_VERSION}"
