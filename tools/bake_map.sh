#!/usr/bin/env bash
# =============================================================================
# bake_map.sh — Unity 맵 에셋 → Entertainment 앱 배포
#
# 선행 조건 (Unity Editor에서 수동 실행):
#   1. CarSim → Bake Styled Map Tiles
#      출력: Assets/StreamingAssets/styled_tiles/{z}/{tx}/{ty}.png
#   2. CarSim → Bake Road Mask  (선택)
#      출력: Assets/StreamingAssets/road_mask.png
#
# 사용법:
#   tools/bake_map.sh [옵션]
#
# 옵션:
#   --unity <경로>   CarSimulatorUnit 프로젝트 경로 (기본: ~/Documents/CarSimulatorUnit)
#   --build <경로>   빌드 출력 경로 (기본: ./build)
#   --no-graph       road_graph.json 생성 건너뜀
#   --package        tar.gz 패키지 생성 (대용량 타일 배포용)
#   --help           도움말
#
# 예시:
#   # 기본 실행 (styled_tiles 복사 + road_graph 생성 + 빌드 심볼릭 링크)
#   tools/bake_map.sh
#
#   # Unity 경로 지정
#   tools/bake_map.sh --unity ~/projects/CarSimulatorUnit
#
#   # Pi 배포용 패키지 생성
#   tools/bake_map.sh --package
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# ── 기본값 ─────────────────────────────────────────────────────────────────────
UNITY_PROJECT="${UNITY_PROJECT:-$HOME/Documents/CarSimulatorUnit}"
BUILD_DIR="${BUILD_DIR:-$REPO_DIR/build}"
MAP_ASSETS_DIR="$REPO_DIR/assets/map"
DO_GRAPH=true
DO_PACKAGE=false

# ── 인수 파싱 ───────────────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --unity)    UNITY_PROJECT="$2"; shift 2 ;;
        --build)    BUILD_DIR="$2";    shift 2 ;;
        --no-graph) DO_GRAPH=false;    shift   ;;
        --package)  DO_PACKAGE=true;   shift   ;;
        --help|-h)
            sed -n '3,30p' "$0" | sed 's/^# \?//'
            exit 0 ;;
        *) echo "알 수 없는 인수: $1  (--help 참고)"; exit 1 ;;
    esac
done

# ── 색상 출력 ──────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GRN='\033[0;32m'; YLW='\033[1;33m'; NC='\033[0m'
info()  { echo -e "${GRN}[INFO]${NC}  $*"; }
warn()  { echo -e "${YLW}[WARN]${NC}  $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*" >&2; exit 1; }
step()  { echo -e "\n${GRN}══ $* ══${NC}"; }

# ── 0. 경로 확인 ───────────────────────────────────────────────────────────────
step "0/4 경로 확인"
info "Unity 프로젝트: $UNITY_PROJECT"
info "빌드 디렉토리:  $BUILD_DIR"
info "에셋 저장 경로: $MAP_ASSETS_DIR"

[[ -d "$UNITY_PROJECT" ]] || error "Unity 프로젝트 없음: $UNITY_PROJECT\n  --unity 옵션으로 경로를 지정하세요."

STREAMING_ASSETS="$UNITY_PROJECT/Assets/StreamingAssets"

# styled_tiles 우선, 없으면 tiles 폴백
TILE_SRC=""
if [[ -d "$STREAMING_ASSETS/styled_tiles" ]] && \
   [[ -n "$(find "$STREAMING_ASSETS/styled_tiles" -name "*.png" -print -quit 2>/dev/null)" ]]; then
    TILE_SRC="$STREAMING_ASSETS/styled_tiles"
    info "타일 소스: styled_tiles (네비게이션 스타일) ✓"
elif [[ -d "$STREAMING_ASSETS/tiles" ]] && \
     [[ -n "$(find "$STREAMING_ASSETS/tiles" -name "*.png" -print -quit 2>/dev/null)" ]]; then
    TILE_SRC="$STREAMING_ASSETS/tiles"
    warn "styled_tiles 없음 — 일반 tiles 사용"
    warn "권장: Unity에서 CarSim → Bake Styled Map Tiles 실행"
else
    echo ""
    echo -e "${RED}타일이 없습니다!${NC}"
    echo ""
    echo "Unity Editor에서 다음 순서로 실행하세요:"
    echo "  1. Unity를 열고 씬 로드"
    echo "  2. 메뉴: CarSim → Bake Styled Map Tiles"
    echo "     (출력: Assets/StreamingAssets/styled_tiles/)"
    echo "  3. (선택) CarSim → Bake Road Mask"
    echo "     (출력: Assets/StreamingAssets/road_mask.png)"
    echo "  4. 이 스크립트 다시 실행"
    echo ""
    exit 1
fi

ROAD_MASK="$STREAMING_ASSETS/road_mask.png"
TILE_COUNT=$(find "$TILE_SRC" -name "*.png" | wc -l)
info "타일 수: $TILE_COUNT"

# ── 1. 타일 복사 ────────────────────────────────────────────────────────────────
step "1/4 타일 복사"
mkdir -p "$MAP_ASSETS_DIR/tiles"
rsync -a --delete --info=progress2 --exclude='*.meta' "$TILE_SRC/" "$MAP_ASSETS_DIR/tiles/"
info "복사 완료: $MAP_ASSETS_DIR/tiles/"

# ── 2. road_graph.json 생성 ─────────────────────────────────────────────────────
step "2/4 road_graph.json"
if [[ "$DO_GRAPH" == true ]]; then
    if [[ -f "$ROAD_MASK" ]]; then
        # Python 의존성 확인
        if ! python3 -c "import cv2, skimage, scipy" 2>/dev/null; then
            warn "Python 패키지 없음 — 설치 시도 중..."
            if command -v pacman &>/dev/null; then
                sudo pacman -S --noconfirm python-opencv python-scikit-image python-scipy
            elif command -v pip3 &>/dev/null; then
                pip3 install opencv-python scikit-image scipy
            elif python3 -m pip &>/dev/null; then
                python3 -m pip install opencv-python scikit-image scipy
            else
                error "pip / pacman 없음\n  sudo pacman -S python-opencv python-scikit-image python-scipy"
            fi
        fi

        BOUNDS_SIDECAR="${ROAD_MASK%.png}.bounds.json"
        if [[ -f "$BOUNDS_SIDECAR" ]]; then
            MIN_X=$(python3 -c "import json; print(json.load(open('$BOUNDS_SIDECAR'))['min_x'])")
            MAX_X=$(python3 -c "import json; print(json.load(open('$BOUNDS_SIDECAR'))['max_x'])")
            MIN_Z=$(python3 -c "import json; print(json.load(open('$BOUNDS_SIDECAR'))['min_z'])")
            MAX_Z=$(python3 -c "import json; print(json.load(open('$BOUNDS_SIDECAR'))['max_z'])")
            info "베이크 sidecar 사용: X[$MIN_X, $MAX_X]  Z[$MIN_Z, $MAX_Z]"
        else
            MIN_X=-400; MAX_X=450; MIN_Z=-200; MAX_Z=240
            warn "road_mask.bounds.json 없음 — 기본값 X[-400,450] Z[-200,240] 사용"
            warn "  Unity에서 Bake Road Mask 다시 실행하면 sidecar가 생성됩니다"
        fi

        WORLD_W=$(python3 -c "print(abs($MAX_X - $MIN_X))")
        WORLD_H=$(python3 -c "print(abs($MAX_Z - $MIN_Z))")
        if (( $(echo "$WORLD_W * $WORLD_H > 2000000" | bc -l) )); then
            WORK_SIZE=4096; THRESHOLD=40
            info "넓은 영역 감지 (${WORLD_W%.*}×${WORLD_H%.*}) → work-size=$WORK_SIZE threshold=$THRESHOLD"
        else
            WORK_SIZE=2048; THRESHOLD=40
        fi

        python3 "$SCRIPT_DIR/extract_road_graph.py" \
            --mask  "$ROAD_MASK" \
            --out   "$MAP_ASSETS_DIR/road_graph.json" \
            --min-x "$MIN_X" --max-x "$MAX_X" \
            --min-z "$MIN_Z" --max-z "$MAX_Z" \
            --work-size "$WORK_SIZE" --threshold "$THRESHOLD"
        info "road_graph.json 생성 완료"
    else
        warn "road_mask.png 없음 — road_graph.json 건너뜀"
        warn "네비게이션 벡터 모드를 쓰려면: Unity → CarSim → Bake Road Mask"
    fi
else
    info "road_graph 건너뜀 (--no-graph)"
fi

# ── 3. 빌드 디렉토리 연결 ───────────────────────────────────────────────────────
step "3/4 빌드 디렉토리 연결"
ENT_BUILD="$BUILD_DIR/apps/entertainment"

if [[ -d "$ENT_BUILD" ]]; then
    # tiles 심볼릭 링크 (재빌드 시마다 복사 불필요)
    TILE_LINK="$ENT_BUILD/tiles"
    if [[ -L "$TILE_LINK" ]]; then rm "$TILE_LINK"
    elif [[ -d "$TILE_LINK" ]]; then rm -rf "$TILE_LINK"; fi
    ln -s "$MAP_ASSETS_DIR/tiles" "$TILE_LINK"
    info "심볼릭 링크: $TILE_LINK → $MAP_ASSETS_DIR/tiles"

    if [[ -f "$MAP_ASSETS_DIR/road_graph.json" ]]; then
        cp "$MAP_ASSETS_DIR/road_graph.json" "$ENT_BUILD/road_graph.json"
        info "road_graph.json 복사됨"
    fi
else
    warn "빌드 디렉토리 없음 ($ENT_BUILD)"
    warn "먼저 빌드 실행:  cmake --build $BUILD_DIR --target entertainment"
fi

# ── 4. .gitignore / 패키지 처리 ─────────────────────────────────────────────────
step "4/4 Git / 패키지"

TILES_MB=$(du -sm "$MAP_ASSETS_DIR/tiles" 2>/dev/null | cut -f1 || echo 0)
info "타일 용량: ~${TILES_MB} MB"

GITIGNORE="$REPO_DIR/.gitignore"
if [[ "$TILES_MB" -gt 50 ]]; then
    warn "타일이 ${TILES_MB}MB로 50MB 초과 — .gitignore에 추가합니다"
    if ! grep -qF "assets/map/tiles/" "$GITIGNORE" 2>/dev/null; then
        printf '\n# 맵 타일 PNG (대용량 — bake_map.sh 또는 배포 패키지로 생성)\nassets/map/tiles/\n' >> "$GITIGNORE"
        warn ".gitignore 업데이트됨 (Git LFS 사용 시: git lfs track 'assets/map/tiles/**/*.png')"
    fi
    DO_PACKAGE=true   # 크면 자동으로 패키지 생성
else
    info "${TILES_MB}MB — git에 직접 커밋 가능합니다"
    # 이전에 gitignore에 있었다면 제거
    if grep -qF "assets/map/tiles/" "$GITIGNORE" 2>/dev/null; then
        sed -i '/assets\/map\/tiles\//d' "$GITIGNORE"
        sed -i '/# 맵 타일 PNG/d' "$GITIGNORE"
        info ".gitignore에서 assets/map/tiles/ 제거됨"
    fi
fi

# road_graph.json 은 항상 git 추적 (크기 작음)
sed -i '/assets\/map\/road_graph.json/d' "$GITIGNORE" 2>/dev/null || true

if [[ "$DO_PACKAGE" == true ]]; then
    PKG="$REPO_DIR/map-assets.tar.gz"
    info "패키지 생성 중: $PKG"
    # road_graph.json 포함 (있는 경우)
    EXTRA_FILES=()
    [[ -f "$MAP_ASSETS_DIR/road_graph.json" ]] && EXTRA_FILES+=("road_graph.json")
    tar -czf "$PKG" -C "$MAP_ASSETS_DIR" tiles "${EXTRA_FILES[@]}"
    PKG_MB=$(du -sm "$PKG" | cut -f1)
    info "패키지 완료: $PKG  (~${PKG_MB} MB)"
    echo ""
    echo "Pi 배포:"
    echo "  scp $PKG pi@<PI_IP>:/tmp/"
    echo "  ssh pi@<PI_IP> 'sudo tar -xzf /tmp/map-assets.tar.gz -C /opt/entertainment/'"
fi

# ── 완료 ──────────────────────────────────────────────────────────────────────
echo ""
echo -e "${GRN}╔══════════════════════════════════════════════════════╗${NC}"
echo -e "${GRN}║   맵 에셋 준비 완료!                                 ║${NC}"
echo -e "${GRN}╚══════════════════════════════════════════════════════╝${NC}"
echo ""
info "타일:       $MAP_ASSETS_DIR/tiles/  ($TILE_COUNT 파일)"
[[ -f "$MAP_ASSETS_DIR/road_graph.json" ]] && \
    info "road_graph: $MAP_ASSETS_DIR/road_graph.json"
echo ""
echo "다음 단계:"
echo "  빌드:   cmake --build $BUILD_DIR --target entertainment"
echo "  실행:   $ENT_BUILD/entertainment"
echo ""
if [[ "$TILES_MB" -le 50 ]]; then
    echo "Git 커밋:"
    echo "  git add assets/map/"
    echo "  git commit -m 'feat: add baked map tiles and road_graph'"
fi
