#!/bin/bash
# Car Cluster OTA Update Script
# 부팅 시 GitHub Releases에서 최신 버전 확인 후 업데이트
set -euo pipefail

INSTALL_DIR="/opt/cluster"
BINARY_NAME="cluster"
ASSET_NAME="cluster-arm64"
GITHUB_REPO="BitByte08/Cockpit.EntertainmentClusterUnit"
API_URL="https://api.github.com/repos/${GITHUB_REPO}/releases/latest"
VERSION_FILE="${INSTALL_DIR}/VERSION"
TMP_BINARY="/tmp/cluster-update"
LOG_TAG="cluster-update"

log() { logger -t "$LOG_TAG" "$*"; echo "[$(date '+%H:%M:%S')] $*"; }

# ── 현재 버전 읽기 ────────────────────────────────────────────────────────────
CURRENT_VERSION="0.0.0"
if [[ -f "$VERSION_FILE" ]]; then
    CURRENT_VERSION=$(tr -d '[:space:]' < "$VERSION_FILE")
fi
log "현재 버전: v${CURRENT_VERSION}"

# ── 네트워크 확인 ─────────────────────────────────────────────────────────────
if ! curl -sf --max-time 5 https://api.github.com > /dev/null; then
    log "네트워크 없음 — 업데이트 건너뜀"
    exit 0
fi

# ── 최신 릴리스 조회 ──────────────────────────────────────────────────────────
RELEASE_JSON=$(curl -sf --max-time 10 \
    -H "Accept: application/vnd.github+json" \
    "$API_URL") || { log "GitHub API 조회 실패"; exit 0; }

LATEST_TAG=$(echo "$RELEASE_JSON" | python3 -c \
    "import sys,json; d=json.load(sys.stdin); print(d.get('tag_name',''))" 2>/dev/null)
LATEST_VERSION="${LATEST_TAG#v}"   # 'v' 접두어 제거

if [[ -z "$LATEST_VERSION" ]]; then
    log "릴리스 정보 파싱 실패"
    exit 0
fi
log "최신 버전: v${LATEST_VERSION}"

# ── 버전 비교 (major.minor.patch 숫자 비교) ──────────────────────────────────
version_gt() {
    # $1 > $2 이면 0 반환
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

# ── 다운로드 URL 추출 ─────────────────────────────────────────────────────────
DOWNLOAD_URL=$(echo "$RELEASE_JSON" | python3 -c "
import sys, json
d = json.load(sys.stdin)
for a in d.get('assets', []):
    if a['name'] == '${ASSET_NAME}':
        print(a['browser_download_url'])
        break
" 2>/dev/null)

if [[ -z "$DOWNLOAD_URL" ]]; then
    log "릴리스에서 '${ASSET_NAME}' 에셋을 찾을 수 없음"
    exit 0
fi

# ── 바이너리 다운로드 ─────────────────────────────────────────────────────────
log "다운로드 중: $DOWNLOAD_URL"
curl -L --max-time 120 --progress-bar -o "$TMP_BINARY" "$DOWNLOAD_URL" || {
    log "다운로드 실패"
    rm -f "$TMP_BINARY"
    exit 0
}

# ── 최소 유효성 검사 (ELF 매직 바이트) ───────────────────────────────────────
MAGIC=$(xxd -l 4 "$TMP_BINARY" 2>/dev/null | awk '{print $2$3}')
if [[ "$MAGIC" != "7f454c46" ]]; then
    log "다운로드된 파일이 유효한 ELF 바이너리가 아님"
    rm -f "$TMP_BINARY"
    exit 0
fi

# ── 교체 및 버전 파일 업데이트 ────────────────────────────────────────────────
chmod +x "$TMP_BINARY"
cp "$TMP_BINARY" "${INSTALL_DIR}/${BINARY_NAME}"
rm -f "$TMP_BINARY"
echo "$LATEST_VERSION" > "$VERSION_FILE"

log "업데이트 완료: v${LATEST_VERSION}"
