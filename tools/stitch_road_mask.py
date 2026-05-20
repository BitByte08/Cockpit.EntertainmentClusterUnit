#!/usr/bin/env python3
"""
Stitch styled map tiles into a road mask PNG for extract_road_graph.py
======================================================================
Reads styled_tiles/<zoom>/<tx>/<ty>.png (Mapbox-style color-coded tiles)
and produces a grayscale road mask where:
  255 = highway  (#FFF3D4 / #D7BE84)
  200 = major road (future use)
  160 = local road (#FFFFFA)
    0 = not a road (buildings #C8D0D8, background #ECEFF2, etc.)

Usage:
  python3 tools/stitch_road_mask.py \
      --tiles /path/to/styled_tiles \
      --zoom  5 \
      --out   /tmp/road_mask_styled.png \
      --size  4096
"""

import argparse
import os
import sys

import numpy as np

try:
    import cv2
except ImportError:
    print("[ERROR] opencv-python not found.  pip install opencv-python")
    sys.exit(1)

# ── 색상 정의 (RGB) ────────────────────────────────────────────────────────────
# 하이웨이 표면 및 중앙분리대
HIGHWAY_RGB  = [(0xFF, 0xF3, 0xD4), (0xD7, 0xBE, 0x84)]
# 시내 도로
LOCAL_RGB    = [(0xFF, 0xFF, 0xFA)]
# 건물 색상 (건물 바닥면 탐지용)
BUILDING_RGB = [(0xDD, 0xE2, 0xE7), (0xC8, 0xD0, 0xD8), (0xC3, 0xC8, 0xCE),
                (0x96, 0xA0, 0xAA), (0x91, 0x9A, 0xA3), (0xB8, 0xC0, 0xC8)]


def detect_roads(tile_bgr: np.ndarray):
    """
    tile_bgr : H×W×3 uint8 BGR (as returned by cv2.imread)
    returns  : (road_mask, building_mask)
      road_mask     : H×W uint8  255=highway, 160=local, 0=non-road
      building_mask : H×W uint8  255=building color pixel, 0=other
    """
    road_mask = np.zeros(tile_bgr.shape[:2], dtype=np.uint8)
    bld_mask  = np.zeros(tile_bgr.shape[:2], dtype=np.uint8)

    b, g, r = tile_bgr[:,:,0], tile_bgr[:,:,1], tile_bgr[:,:,2]

    for rr, rg, rb in HIGHWAY_RGB:
        hit = (r == rr) & (g == rg) & (b == rb)
        road_mask[hit] = 255

    for rr, rg, rb in LOCAL_RGB:
        hit = (r == rr) & (g == rg) & (b == rb)
        road_mask[hit] = np.maximum(road_mask[hit], 160)

    for rr, rg, rb in BUILDING_RGB:
        hit = (r == rr) & (g == rg) & (b == rb)
        bld_mask[hit] = 255

    return road_mask, bld_mask


def stitch(tiles_root: str, zoom: int, out_size: int):
    """
    returns (road_out, building_out) — both H×W uint8
    """
    n = 1 << zoom          # tiles per side
    step = out_size // n   # output pixels per tile

    if step < 1:
        raise ValueError(f"out_size {out_size} is too small for zoom {zoom} ({n} tiles)")

    out     = np.zeros((out_size, out_size), dtype=np.uint8)
    bld_out = np.zeros((out_size, out_size), dtype=np.uint8)
    total = n * n
    done = 0

    for tx in range(n):
        for ty in range(n):
            path = os.path.join(tiles_root, str(zoom), str(tx), f"{ty}.png")
            if os.path.exists(path):
                tile_bgr = cv2.imread(path, cv2.IMREAD_COLOR)
                if tile_bgr is None:
                    done += 1
                    continue

                # 도로 마스크 + 건물 마스크 (전체 해상도)
                road_mask, bld_mask = detect_roads(tile_bgr)

                # 하이웨이 / 로컬 마스크 개별 축소
                hwy_full   = (road_mask == 255).astype(np.uint8) * 255
                local_full = (road_mask == 160).astype(np.uint8) * 160

                hwy_small   = cv2.resize(hwy_full,   (step, step), interpolation=cv2.INTER_AREA)
                local_small = cv2.resize(local_full, (step, step), interpolation=cv2.INTER_AREA)
                bld_small   = cv2.resize(bld_mask,   (step, step), interpolation=cv2.INTER_AREA)

                _, hwy_bin   = cv2.threshold(hwy_small,   10, 255, cv2.THRESH_BINARY)
                _, local_bin = cv2.threshold(local_small, 10, 160, cv2.THRESH_BINARY)
                _, bld_bin   = cv2.threshold(bld_small,   10, 255, cv2.THRESH_BINARY)

                tile_out = np.maximum(hwy_bin, local_bin)
                out[ty * step:(ty + 1) * step, tx * step:(tx + 1) * step] = tile_out
                bld_out[ty * step:(ty + 1) * step, tx * step:(tx + 1) * step] = bld_bin

            done += 1
            if done % 64 == 0 or done == total:
                pct = done / total * 100
                print(f"\r  타일 처리: {done}/{total} ({pct:.0f}%)", end="", flush=True)

    print()
    return out, bld_out


def remove_isolated_components(out: np.ndarray) -> np.ndarray:
    """
    하이웨이 링에 연결되지 않은 고립 컴포넌트(건물 내부 바닥 등)를 제거합니다.

    원리:
    - 도로(local + highway)를 하나의 연결 그래프로 봤을 때,
      건물 내부 픽셀들은 건물 벽(비-#FFFFFA)에 둘러싸여 있어 하이웨이와 단절됩니다.
    - 하이웨이 픽셀과 연결된 컴포넌트만 남기고 나머지를 제거합니다.
    """
    from scipy.ndimage import label as scipy_label

    # 모든 도로 픽셀(highway + local) → 이진 마스크
    binary = (out > 0).astype(np.uint8)

    # 8방향 연결 레이블링
    conn8 = np.ones((3, 3), dtype=int)
    labeled, num = scipy_label(binary, structure=conn8)

    # 하이웨이 픽셀(255)이 포함된 컴포넌트 ID 수집
    hwy_pixels = (out == 255)
    highway_labels = set(np.unique(labeled[hwy_pixels]))
    highway_labels.discard(0)

    # 하이웨이와 연결된 컴포넌트만 보존
    keep = np.zeros_like(binary, dtype=bool)
    for lbl in highway_labels:
        keep |= (labeled == lbl)

    removed = int(np.sum(binary & ~keep))
    result = out.copy()
    result[~keep] = 0
    print(f"      고립 컴포넌트 제거: {removed}  (하이웨이 연결 컴포넌트 {len(highway_labels)}개 유지)")
    return result


def remove_exterior_terrain(out: np.ndarray) -> np.ndarray:
    """
    하이웨이 링 외부의 지형(#FFFFFA 로 채워진 넓은 영역)을 제거합니다.

    원리:
     1. 하이웨이 픽셀(255)을 두텁게 팽창 → 하이웨이 링이 완전히 닫힌 장벽이 됨
     2. 이미지 경계에서 시작해 로컬도로 픽셀(160)을 BFS 플러드필
        → 하이웨이 장벽을 넘지 못하므로 외부 지형만 채워짐
     3. 플러드필된 영역을 마스크에서 제거
    """
    H, W = out.shape
    hwy_mask   = (out == 255).astype(np.uint8)
    local_mask = (out == 160).astype(np.uint8)

    # 하이웨이를 충분히 팽창시켜 끊긴 부분 봉합 (30px ≈ 약 6 세계 좌표 단위)
    barrier_r = max(15, H // 130)
    barrier = cv2.dilate(
        hwy_mask,
        cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (barrier_r * 2 + 1, barrier_r * 2 + 1))
    )

    # 플러드필 대상: 로컬도로 AND NOT 하이웨이 장벽
    fill_target = ((local_mask > 0) & (barrier == 0)).astype(np.uint8) * 255

    # 이미지 테두리에서 BFS: 테두리의 fill_target 픽셀들을 씨앗으로 사용
    # cv2.floodFill은 단일 씨앗만 지원하므로 scipy.ndimage.label 사용
    from scipy.ndimage import label as scipy_label
    labeled, num = scipy_label(fill_target)

    # 이미지 경계에 닿는 컴포넌트 = 외부 지형
    border_labels = set()
    border_labels.update(labeled[0,   :].flat)
    border_labels.update(labeled[-1,  :].flat)
    border_labels.update(labeled[:,  0].flat)
    border_labels.update(labeled[:, -1].flat)
    border_labels.discard(0)

    exterior = np.zeros_like(local_mask, dtype=bool)
    for lbl in border_labels:
        exterior |= (labeled == lbl)

    # 외부 지형 제거
    result = out.copy()
    result[exterior] = 0
    removed = int(np.sum(exterior))
    print(f"      외부 지형 픽셀 제거: {removed}")
    return result


def main():
    ap = argparse.ArgumentParser(description="Stitch styled tiles → road mask PNG")
    ap.add_argument("--tiles", required=True, help="styled_tiles 루트 경로")
    ap.add_argument("--zoom",  type=int, default=5, help="줌 레벨 (기본 5)")
    ap.add_argument("--out",   required=True, help="출력 PNG 경로")
    ap.add_argument("--size",  type=int, default=4096,
                    help="출력 이미지 크기 (픽셀, 기본 4096).  zoom 레벨 타일 수의 배수여야 함")
    args = ap.parse_args()

    n = 1 << args.zoom
    if args.size % n != 0:
        adjusted = (args.size // n) * n
        print(f"[warn] --size {args.size} is not a multiple of {n}; adjusted to {adjusted}")
        args.size = adjusted

    print(f"[1/3] 타일 스티칭: zoom={args.zoom}  ({n}×{n} 타일, 출력 {args.size}×{args.size}px)")
    out, _bld_out = stitch(args.tiles, args.zoom, args.size)

    road_px = np.sum(out > 0)
    hwy_px  = np.sum(out == 255)
    print(f"      도로 픽셀: {road_px}  (하이웨이={hwy_px}, 시내={road_px - hwy_px})")

    print(f"[2/3] 고립 컴포넌트 제거 (건물 내부 + 외부 지형)...")
    out = remove_isolated_components(out)

    road_px = np.sum(out > 0)
    hwy_px  = np.sum(out == 255)
    print(f"      정리 후 도로 픽셀: {road_px}  (하이웨이={hwy_px}, 시내={road_px - hwy_px})")

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    cv2.imwrite(args.out, out)
    size_kb = os.path.getsize(args.out) // 1024
    print(f"[3/3] 저장 완료: {args.out}  ({size_kb} KB)")
    print()
    print("다음 단계:")
    print(f"  python3 extract_road_graph.py \\")
    print(f"      --mask  {args.out} \\")
    print(f"      --out   assets/map/road_graph.json \\")
    print(f"      --min-x -400 --max-x 450 --min-z -200 --max-z 240 \\")
    print(f"      --work-size {args.size} --threshold 100")


if __name__ == "__main__":
    main()
