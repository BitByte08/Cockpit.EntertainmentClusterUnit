#!/usr/bin/env python3
"""
Road Graph Extractor
====================
road_mask.png (Unity RoadMaskBaker 출력) →  road_graph.json

도로 픽셀 밝기로 등급 구분:
  brightness >= 0.9  →  "highway"
  brightness >= 0.75 →  "major"
  그 외              →  "local"

사용법:
  pip install opencv-python scikit-image numpy scipy
  python3 tools/extract_road_graph.py \
      --mask   ../CarSimulatorUnit/Assets/StreamingAssets/road_mask.png \
      --out    build/apps/entertainment/road_graph.json \
      --min-x  -400 --max-x 450 --min-z -200 --max-z 240

결과 JSON:
  {
    "world_bounds": {"min_x":-400,"max_x":450,"min_z":-200,"max_z":240},
    "nodes": [{"id":0,"x":12.3,"z":45.6}, ...],
    "edges": [{"from":0,"to":1,"type":"highway",
               "points":[{"x":...,"z":...}, ...]}, ...]
  }
"""
import sys
import json
import argparse
import numpy as np

def check_deps():
    missing = []
    for pkg, imp in [("opencv-python","cv2"),("scikit-image","skimage"),("scipy","scipy")]:
        try: __import__(imp)
        except ImportError: missing.append(pkg)
    if missing:
        print(f"[ERROR] 패키지 없음: {', '.join(missing)}")
        print(f"  pip install {' '.join(missing)}")
        sys.exit(1)

check_deps()

import cv2
from skimage.morphology import skeletonize
from scipy.ndimage import convolve, label

# ── 유틸 ──────────────────────────────────────────────────────────────────────

def px_to_world(px, py, bounds, img_h, img_w):
    """픽셀 좌표 → 월드 XZ (이미지 y축이 +Z방향 위쪽)"""
    wx = bounds["min_x"] + px / img_w * (bounds["max_x"] - bounds["min_x"])
    wz = bounds["max_z"] - py / img_h * (bounds["max_z"] - bounds["min_z"])
    return wx, wz

def world_to_px(wx, wz, bounds, img_h, img_w):
    px = (wx - bounds["min_x"]) / (bounds["max_x"] - bounds["min_x"]) * img_w
    py = (bounds["max_z"] - wz) / (bounds["max_z"] - bounds["min_z"]) * img_h
    return int(px), int(py)

# ── 메인 처리 ─────────────────────────────────────────────────────────────────

def extract_graph(mask_path, bounds, work_size=2048, min_edge_len=3):
    print(f"[1/6] 마스크 로드: {mask_path}")
    img = cv2.imread(mask_path, cv2.IMREAD_GRAYSCALE)
    if img is None:
        raise FileNotFoundError(f"파일을 열 수 없음: {mask_path}")
    orig_h, orig_w = img.shape
    print(f"      원본 크기: {orig_w}×{orig_h}")

    # 도로 등급 판별용 원본 평균 밝기 보존 (리사이즈 전)
    scale = work_size / max(orig_w, orig_h)
    ww = int(orig_w * scale)
    wh = int(orig_h * scale)

    print(f"[2/6] 리사이즈: {ww}×{wh}, 전처리")
    img_r = cv2.resize(img, (ww, wh), interpolation=cv2.INTER_AREA)

    # 밝기 맵 보존 (등급 판별용)
    bright_map = img_r.astype(np.float32) / 255.0

    # 가우시안 블러 → 이진화
    blurred = cv2.GaussianBlur(img_r, (5, 5), 0)
    _, binary = cv2.threshold(blurred, 40, 1, cv2.THRESH_BINARY)
    binary = binary.astype(bool)

    print(f"[3/6] 스켈레톤화 (Zhang-Suen)...")
    skel = skeletonize(binary).astype(np.uint8)
    print(f"      도로 픽셀: {skel.sum()}")

    # ── 노드 검출 (접속 도로 수 ≠ 2 인 픽셀) ─────────────────────────────────
    print(f"[4/6] 교차점/끝점 검출...")
    kernel = np.array([[1,1,1],[1,0,1],[1,1,1]], dtype=np.uint8)
    degree = convolve(skel, kernel, mode='constant') * skel
    # 교차점: degree >= 3  /  끝점: degree == 1  /  고립: degree == 0
    special_mask = ((degree >= 3) | (degree == 1)) & (skel > 0)

    # 교차점이 뭉쳐 있으면 하나의 노드로 합치기 (모폴로지 팽창 → 라벨링)
    ksize = max(3, int(12 * scale))  # 약 6 world-unit 반경
    dilated = cv2.dilate(special_mask.astype(np.uint8),
                         cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (ksize, ksize)))
    labeled, num = label(dilated)

    nodes = []
    node_pos_px = []  # (row, col) pixel of each node
    node_map = np.full(skel.shape, -1, dtype=np.int32)  # pixel → node id

    for nid in range(1, num + 1):
        ys, xs = np.where((labeled == nid) & special_mask)
        if len(ys) == 0:
            continue
        cy, cx = int(ys.mean()), int(xs.mean())
        wx, wz = px_to_world(cx, cy, bounds, wh, ww)
        nidx = len(nodes)
        nodes.append({"id": nidx, "x": round(wx, 2), "z": round(wz, 2)})
        node_pos_px.append((cy, cx))
        for y, x in zip(ys, xs):
            node_map[y, x] = nidx

    print(f"      노드 수: {len(nodes)}")

    # ── 엣지 추적 (BFS) ──────────────────────────────────────────────────────
    print(f"[5/6] 엣지 추적...")
    visited_edge = np.zeros(skel.shape, dtype=bool)
    edges = []
    dirs = [(-1,-1),(-1,0),(-1,1),(0,-1),(0,1),(1,-1),(1,0),(1,1)]

    def road_type(brightness_val):
        if brightness_val >= 0.88: return "highway"
        if brightness_val >= 0.73: return "major"
        return "local"

    for start_nid, (sr, sc) in enumerate(node_pos_px):
        # 이 노드의 모든 스켈레톤 픽셀에서 BFS 출발
        start_pixels = list(zip(*np.where(labeled == (start_nid + 1))))
        start_pixels = [(r, c) for r, c in start_pixels if skel[r, c]]

        for (r0, c0) in start_pixels:
            for (dr, dc) in dirs:
                nr, nc = r0 + dr, c0 + dc
                if not (0 <= nr < wh and 0 <= nc < ww): continue
                if not skel[nr, nc]: continue
                if visited_edge[nr, nc]: continue
                if node_map[nr, nc] == start_nid: continue

                # 이 픽셀에서 BFS로 다음 노드까지 경로 추적
                path_px = [(r0, c0), (nr, nc)]
                brightness_sum = bright_map[r0, c0] + bright_map[nr, nc]
                visited_edge[nr, nc] = True
                cur_r, cur_c = nr, nc
                prev_r, prev_c = r0, c0

                found_nid = node_map[nr, nc]

                while found_nid < 0:
                    moved = False
                    for (dr2, dc2) in dirs:
                        ar, ac = cur_r + dr2, cur_c + dc2
                        if not (0 <= ar < wh and 0 <= ac < ww): continue
                        if not skel[ar, ac]: continue
                        if visited_edge[ar, ac]: continue
                        if ar == prev_r and ac == prev_c: continue
                        visited_edge[ar, ac] = True
                        brightness_sum += bright_map[ar, ac]
                        path_px.append((ar, ac))
                        prev_r, prev_c = cur_r, cur_c
                        cur_r, cur_c = ar, ac
                        found_nid = node_map[ar, ac]
                        moved = True
                        break
                    if not moved:
                        break  # 막힌 경로 (노드 없이 끝)

                if found_nid < 0 or found_nid == start_nid:
                    continue
                if len(path_px) < min_edge_len:
                    continue

                avg_brightness = brightness_sum / len(path_px)
                rtype = road_type(avg_brightness)

                # 경로 단순화 (Douglas-Peucker)
                pts = np.array([[c, r] for r, c in path_px], dtype=np.float32).reshape(-1, 1, 2)
                epsilon = max(2, int(6 * scale))
                approx = cv2.approxPolyDP(pts, epsilon, False)
                world_pts = []
                for pt in approx:
                    px_, py_ = int(pt[0][0]), int(pt[0][1])
                    wx, wz = px_to_world(px_, py_, bounds, wh, ww)
                    world_pts.append({"x": round(wx, 2), "z": round(wz, 2)})

                edges.append({
                    "from": start_nid,
                    "to":   found_nid,
                    "type": rtype,
                    "points": world_pts,
                })

    print(f"      엣지 수: {len(edges)}")

    # ── 고립 노드 제거 ─────────────────────────────────────────────────────────
    connected = set()
    for e in edges:
        connected.add(e["from"])
        connected.add(e["to"])
    id_remap = {}
    new_nodes = []
    for n in nodes:
        if n["id"] in connected:
            new_id = len(new_nodes)
            id_remap[n["id"]] = new_id
            new_nodes.append({"id": new_id, "x": n["x"], "z": n["z"]})
    for e in edges:
        e["from"] = id_remap[e["from"]]
        e["to"]   = id_remap[e["to"]]

    print(f"      (고립 노드 제거 후) 노드: {len(new_nodes)}, 엣지: {len(edges)}")
    return new_nodes, edges


# ── 진입점 ────────────────────────────────────────────────────────────────────

def main():
    ap = argparse.ArgumentParser(description="Road mask → road_graph.json")
    ap.add_argument("--mask",  required=True, help="road_mask.png 경로")
    ap.add_argument("--out",   required=True, help="출력 road_graph.json 경로")
    ap.add_argument("--min-x", type=float, default=-400)
    ap.add_argument("--max-x", type=float, default= 450)
    ap.add_argument("--min-z", type=float, default=-200)
    ap.add_argument("--max-z", type=float, default= 240)
    ap.add_argument("--work-size", type=int, default=2048,
                    help="처리 해상도 (클수록 정밀하지만 느림, 기본 2048)")
    args = ap.parse_args()

    bounds = {"min_x": args.min_x, "max_x": args.max_x,
              "min_z": args.min_z, "max_z": args.max_z}

    nodes, edges = extract_graph(args.mask, bounds, args.work_size)

    graph = {
        "world_bounds": bounds,
        "nodes": nodes,
        "edges": edges,
    }

    import os
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as f:
        json.dump(graph, f, ensure_ascii=False, separators=(",", ":"))

    size_kb = os.path.getsize(args.out) // 1024
    print(f"[6/6] 저장 완료: {args.out}  ({size_kb} KB)")
    print()
    print("다음 단계: Qt 앱 실행 시 road_graph.json 이 자동으로 로드됩니다.")

if __name__ == "__main__":
    main()
