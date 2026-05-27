#!/bin/bash
# Car Cluster OTA Update Script
# 부팅 시 GitHub Releases에서 최신 버전 확인 후 업데이트
# API 호출 없이 releases/latest/download 직접 다운로드 (rate limit 우회)
set -euo pipefail

INSTALL_DIR="/opt/cluster"
BINARY_NAME="cluster"
ASSET_NAME="cluster-arm64"
GITHUB_REPO="BitByte08/Cockpit.EntertainmentClusterUnit"
DOWNLOAD_BASE="https://github.com/${GITHUB_REPO}/releases/latest/download"
VERSION_FILE="${INSTALL_DIR}/VERSION"
TMP_BINARY="/tmp/cluster-update"
TMP_VERSION="/tmp/cluster-latest-version"
LOG_TAG="cluster-update"

log() { logger -t "$LOG_TAG" "$*"; echo "[$(date '+%H:%M:%S')] $*"; }

# ── 현재 버전 읽기 ────────────────────────────────────────────────────────────
CURRENT_VERSION="0.0.0"
if [[ -f "$VERSION_FILE" ]]; then
    CURRENT_VERSION=$(tr -d '[:space:]' < "$VERSION_FILE")
fi
log "현재 버전: v${CURRENT_VERSION}"

# ── 네트워크 확인 (github.com 직접, API 아님) ─────────────────────────────────
if ! curl -sfL --max-time 5 -H "User-Agent: cluster-update" \
    "https://github.com" > /dev/null; then
    log "네트워크 없음 — 업데이트 건너뜀"
    exit 0
fi

# ── 최신 VERSION 파일 다운로드 (API 없이) ─────────────────────────────────────
rm -f "$TMP_VERSION"
if ! curl -sfL --max-time 10 -H "User-Agent: cluster-update" \
    "${DOWNLOAD_BASE}/VERSION" -o "$TMP_VERSION"; then
    log "릴리스 조회 실패"
    rm -f "$TMP_VERSION"
    exit 0
fi
LATEST_VERSION=$(tr -d '[:space:]' < "$TMP_VERSION")
rm -f "$TMP_VERSION"

if [[ -z "$LATEST_VERSION" ]]; then
    log "릴리스 정보 파싱 실패"
    exit 0
fi
log "최신 버전: v${LATEST_VERSION}"

# ── 버전 비교 (major.minor.patch 숫자 비교) ──────────────────────────────────
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
    log "이미 최신 버전입니다 — 업데이트 불필요"
    exit 0
fi

log "업데이트 발견: v${CURRENT_VERSION} → v${LATEST_VERSION}"

# ── 바이너리 다운로드 (API 없이) ───────────────────────────────────────────────
log "다운로드 중: ${DOWNLOAD_BASE}/${ASSET_NAME}"
if ! curl -fL --max-time 120 --progress-bar -H "User-Agent: cluster-update" \
    "${DOWNLOAD_BASE}/${ASSET_NAME}" -o "$TMP_BINARY"; then
    log "다운로드 실패"
    rm -f "$TMP_BINARY"
    exit 0
fi

# ── 최소 유효성 검사 (ELF 매직 바이트) ───────────────────────────────────────
MAGIC=$(xxd -l 4 "$TMP_BINARY" 2>/dev/null | awk '{print $2$3}')
if [[ "$MAGIC" != "7f454c46" ]]; then
    log "다운로드된 파일이 유효한 ELF 바이너리가 아님"
    rm -f "$TMP_BINARY"
    exit 0
fi

# ── 서비스 중지 후 교체 ──────────────────────────────────────────────────────
systemctl stop cluster-kiosk.service 2>/dev/null || true

chmod +x "$TMP_BINARY"
cp "$TMP_BINARY" "${INSTALL_DIR}/${BINARY_NAME}"
rm -f "$TMP_BINARY"
echo "$LATEST_VERSION" > "$VERSION_FILE"

# 부팅 중이면 서비스 유닛이 다시 시작하지만, 수동 실행 중이면 재시작
if ! systemctl is-system-running 2>/dev/null | grep -q 'booting'; then
    systemctl start cluster-kiosk.service 2>/dev/null || true
fi

log "업데이트 완료: v${LATEST_VERSION}"
