#!/usr/bin/env python3
"""
make_road_mask.py  —  road_graph.json 생성을 위한 도로 마스크 합성
=====================================================================
두 소스를 합쳐 최종 road_mask_combined.png 를 만듭니다:

  1. road_mask.png  (city_part_collider 베이크)
       → 시내 도로 정확한 기하학, 건물 없음
  2. styled_tiles/<zoom>/  (Mapbox 색상 타일)
       → 하이웨이 링: #FFF3D4 / #D7BE84 색상만 추출

사용법:
  python3 tools/make_road_mask.py \
      --city-mask  /path/to/road_mask.png \
      --tiles      /path/to/styled_tiles \
      --zoom       5 \
      --out        /tmp/road_mask_combined.png \
      --size       4096

이후 extract_road_graph.py 를 실행:
  python3 tools/extract_road_graph.py \
      --mask  /tmp/road_mask_combined.png \
      --out   assets/map/road_graph.json \
      --min-x -400 --max-x 450 --min-z -200 --max-z 240 \
      --work-size 4096 --threshold 50
"""

import argparse
import os
import sys

import numpy as np

try:
    import cv2
except ImportError:
    print("[ERROR] opencv-python 없음. pip install opencv-python")
    sys.exit(1)

# ── 하이웨이 색상 (RGB) ────────────────────────────────────────────────────────
HIGHWAY_RGB = [(0xFF, 0xF3, 0xD4), (0xD7, 0xBE, 0x84)]


def stitch_highway(tiles_root: str, zoom: int, out_size: int) -> np.ndarray:
    """styled tiles 에서 하이웨이 픽셀만 추출해 합성"""
    n = 1 << zoom
    step = out_size // n
    out = np.zeros((out_size, out_size), dtype=np.uint8)
    total = n * n
    done = 0

    for tx in range(n):
        for ty in range(n):
            path = os.path.join(tiles_root, str(zoom), str(tx), f"{ty}.png")
            if os.path.exists(path):
                tile = cv2.imread(path, cv2.IMREAD_COLOR)
                if tile is None:
                    done += 1
                    continue
                b, g, r = tile[:, :, 0], tile[:, :, 1], tile[:, :, 2]
                hwy = np.zeros(tile.shape[:2], dtype=np.uint8)
                for rr, rg, rb in HIGHWAY_RGB:
                    hwy[(r == rr) & (g == rg) & (b == rb)] = 255
                small = cv2.resize(hwy, (step, step), interpolation=cv2.INTER_AREA)
                _, small_bin = cv2.threshold(small, 10, 255, cv2.THRESH_BINARY)
                out[ty * step:(ty + 1) * step, tx * step:(tx + 1) * step] = small_bin
            done += 1
            if done % 128 == 0 or done == total:
                print(f"\r  타일: {done}/{total}", end="", flush=True)

    print()
    return out


def main():
    ap = argparse.ArgumentParser(description="road_mask + styled_tiles → combined road mask")
    ap.add_argument("--city-mask", required=True, help="city_part_collider road_mask.png 경로")
    ap.add_argument("--tiles",     required=True, help="styled_tiles 루트 경로")
    ap.add_argument("--zoom",  type=int, default=5,    help="줌 레벨 (기본 5)")
    ap.add_argument("--out",       required=True, help="출력 PNG 경로")
    ap.add_argument("--size",  type=int, default=4096, help="출력 크기 (기본 4096)")
    args = ap.parse_args()

    # ── 1. 시내 도로 마스크 로드 ───────────────────────────────────────────────
    print(f"[1/3] 시내 도로 마스크 로드: {args.city_mask}")
    city = cv2.imread(args.city_mask, cv2.IMREAD_GRAYSCALE)
    if city is None:
        print(f"[ERROR] 파일을 열 수 없음: {args.city_mask}")
        sys.exit(1)
    city = cv2.resize(city, (args.size, args.size), interpolation=cv2.INTER_AREA)
    print(f"      크기: {args.city_mask} → {city.shape[1]}×{city.shape[0]}")

    # ── 2. 하이웨이 스티칭 ────────────────────────────────────────────────────
    n = 1 << args.zoom
    print(f"[2/3] 하이웨이 스티칭: zoom={args.zoom}  ({n}×{n} 타일)")
    highway = stitch_highway(args.tiles, args.zoom, args.size)
    print(f"      하이웨이 픽셀 (원본): {np.sum(highway > 0)}")

    # 두 차선 → 단일 밴드: 모폴로지 클로징으로 중앙 분리대 공백을 채워
    # 스켈레톤이 1개의 중심선만 만들도록 함
    close_r = max(25, args.size // 160)   # ~25px @ 4096 ≈ 5 월드 단위 (두 차선 병합)
    hwy_close = cv2.morphologyEx(
        highway,
        cv2.MORPH_CLOSE,
        cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (close_r * 2 + 1, close_r * 2 + 1))
    )
    print(f"      하이웨이 픽셀 (클로징 후): {np.sum(hwy_close > 0)}")
    highway = hwy_close

    # ── 3. 합성 ──────────────────────────────────────────────────────────────
    # 밝기로 도로 등급 판별: 255=highway(≥0.88), 200=major(≥0.73)
    # 순서: highway ring 먼저, 그 위에 city road 덮기
    # → on-ramp(시내→하이웨이 진입로) 픽셀이 highway 클로징에 덮히지 않음
    print(f"[3/3] 합성 및 저장...")
    combined = np.zeros((args.size, args.size), dtype=np.uint8)
    combined[highway > 0] = 255   # 하이웨이 ring (클로징된 단일 밴드)
    combined[city > 30]   = 200   # 시내 도로가 위에 덮음 → on-ramp 보존

    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    cv2.imwrite(args.out, combined)
    size_kb = os.path.getsize(args.out) // 1024
    print(f"      저장: {args.out}  ({size_kb} KB)")
    print(f"      픽셀: highway={np.sum(combined==255)}, city={np.sum(combined==200)}")
    print()
    print("다음 단계:")
    print(f"  python3 tools/extract_road_graph.py \\")
    print(f"      --mask  {args.out} \\")
    print(f"      --out   assets/map/road_graph.json \\")
    print(f"      --min-x -400 --max-x 450 --min-z -200 --max-z 240 \\")
    print(f"      --work-size {args.size} --threshold 50")


if __name__ == "__main__":
    main()
