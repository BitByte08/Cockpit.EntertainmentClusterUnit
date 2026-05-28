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

def extract_graph(mask_path, bounds, work_size=2048, min_edge_len=3, threshold=40):
    print(f"[1/6] 마스크 로드: {mask_path}")
    # city_part_collider 메시를 흰색으로 렌더한 grayscale 마스크
    img = cv2.imread(mask_path, cv2.IMREAD_GRAYSCALE)
    if img is None:
        raise FileNotFoundError(f"파일을 열 수 없음: {mask_path}")
    orig_h, orig_w = img.shape
    print(f"      원본 크기: {orig_w}×{orig_h}")

    scale = work_size / max(orig_w, orig_h)
    ww = int(orig_w * scale)
    wh = int(orig_h * scale)

    print(f"[2/6] 리사이즈: {ww}×{wh}, 전처리")
    img_r = cv2.resize(img, (ww, wh), interpolation=cv2.INTER_AREA)

    # 밝기 맵 (도로 등급 판별용 — 현재는 모두 흰색이므로 전부 highway)
    bright_map = img_r.astype(np.float32) / 255.0

    # 블러 없이 직접 이진화 → on-ramp 차선 구분선 보존
    _, binary = cv2.threshold(img_r, threshold, 1, cv2.THRESH_BINARY)
    binary = binary.astype(bool)
    print(f"      임계값: {threshold}  도로 픽셀: {binary.sum()}")

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
    node_pos_px = []   # (row, col) pixel of each node
    node_label_id = [] # actual labeled[] index for this node, -1 for injected nodes
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
        node_label_id.append(nid)  # remember which label this node came from
        for y, x in zip(ys, xs):
            node_map[y, x] = nidx

    print(f"      노드 수: {len(nodes)}")

    # ── 폐루프 감지: 노드 없는 스켈레톤 컴포넌트에 임의 노드 삽입 ─────────────
    # 하이웨이처럼 교차점 없는 폐곡선은 degree>=3 픽셀이 없어 노드가 생기지 않음.
    # 8방향 연결(대각선 포함)로 라벨링해야 스켈레톤이 쪼개지지 않음.
    conn8 = np.ones((3, 3), dtype=int)
    skel_comp, skel_n = label(skel, structure=conn8)
    for comp_id in range(1, skel_n + 1):
        ys_c, xs_c = np.where(skel_comp == comp_id)
        if len(ys_c) == 0:
            continue
        # 너무 짧은 노이즈 파편은 건너뜀 (실제 도로 루프는 수백 픽셀 이상)
        if len(ys_c) < 80:
            continue
        # 이미 노드가 있는 컴포넌트는 건너뜀
        has_node = np.any(node_map[ys_c, xs_c] >= 0)
        if has_node:
            continue
        # 노드 없는 폐루프 → 중간 지점을 노드로 삽입
        mid = len(ys_c) // 2
        cy, cx = int(ys_c[mid]), int(xs_c[mid])
        wx, wz = px_to_world(cx, cy, bounds, wh, ww)
        nidx = len(nodes)
        nodes.append({"id": nidx, "x": round(wx, 2), "z": round(wz, 2)})
        node_pos_px.append((cy, cx))
        node_label_id.append(-1)   # injected — not in labeled[]
        node_map[cy, cx] = nidx
        print(f"      폐루프 노드 삽입: id={nidx} ({wx:.1f}, {wz:.1f})  픽셀수={len(ys_c)}")

    print(f"      폐루프 처리 후 노드 수: {len(nodes)}")

    # ── 엣지 추적 (BFS) ──────────────────────────────────────────────────────
    print(f"[5/6] 엣지 추적...")
    visited_edge = np.zeros(skel.shape, dtype=bool)
    edges = []
    dirs = [(-1,-1),(-1,0),(-1,1),(0,-1),(0,1),(1,-1),(1,0),(1,1)]

    def road_type(brightness_val):
        # road_mask.png 는 순수 이진(255/0)이라 밝기로 등급 구분 불가.
        # 기본값 "major"로 설정하고, 뒤의 스팬 기반 highway 승격이 처리.
        if brightness_val >= 0.88: return "major"   # pure mask → all 1.0 → "major"
        if brightness_val >= 0.73: return "major"
        return "local"

    for start_nid, (sr, sc) in enumerate(node_pos_px):
        # 이 노드의 모든 스켈레톤 픽셀에서 BFS 출발
        # node_label_id[start_nid]가 없거나 -1이면(폐루프 삽입 노드), 단일 픽셀만 사용
        nlid = node_label_id[start_nid] if start_nid < len(node_label_id) else -1
        if nlid > 0:
            start_pixels = list(zip(*np.where(labeled == nlid)))
            start_pixels = [(r, c) for r, c in start_pixels if skel[r, c]]
        else:
            start_pixels = [(sr, sc)] if (0 <= sr < wh and 0 <= sc < ww and skel[sr, sc]) else []

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
                        if ar == prev_r and ac == prev_c: continue
                        # 노드 픽셀이면 방문 여부와 무관하게 종료 (on-ramp 접합부 도달)
                        tmp_nid = node_map[ar, ac]
                        if tmp_nid >= 0:
                            path_px.append((ar, ac))
                            brightness_sum += bright_map[ar, ac]
                            found_nid = tmp_nid
                            moved = True
                            break
                        if visited_edge[ar, ac]: continue
                        visited_edge[ar, ac] = True
                        brightness_sum += bright_map[ar, ac]
                        path_px.append((ar, ac))
                        prev_r, prev_c = cur_r, cur_c
                        cur_r, cur_c = ar, ac
                        moved = True
                        break
                    if not moved:
                        break  # 막힌 경로 (노드 없이 끝)

                if found_nid < 0:
                    continue
                if len(path_px) < min_edge_len:
                    continue

                # 자기루프(self-loop): 같은 노드로 돌아오는 긴 경로
                # → 중간 지점마다 노드 삽입 후 분할 (highway 링 등 처리)
                if found_nid == start_nid:
                    # 너무 짧은 루프는 노이즈이므로 건너뜀
                    if len(path_px) < 150:
                        continue
                    # 긴 루프는 N 등분해서 각 분할점을 새 노드로 삽입
                    n_splits = max(2, len(path_px) // 200)
                    loop_nids = [start_nid]
                    for si in range(1, n_splits):
                        mid_idx = si * len(path_px) // n_splits
                        cy, cx = path_px[mid_idx]
                        wx_m, wz_m = px_to_world(cx, cy, bounds, wh, ww)
                        new_nid = len(nodes)
                        nodes.append({"id": new_nid, "x": round(wx_m, 2), "z": round(wz_m, 2)})
                        node_pos_px.append((cy, cx))
                        node_label_id.append(-1)   # injected
                        node_map[cy, cx] = new_nid
                        loop_nids.append(new_nid)
                    loop_nids.append(start_nid)
                    # 분할된 세그먼트를 엣지로 추가
                    seg_boundaries = [0] + [
                        si * len(path_px) // n_splits for si in range(1, n_splits)
                    ] + [len(path_px) - 1]
                    avg_brightness = brightness_sum / len(path_px)
                    rtype = road_type(avg_brightness)
                    for si in range(len(loop_nids) - 1):
                        seg = path_px[seg_boundaries[si]:seg_boundaries[si + 1] + 1]
                        if len(seg) < min_edge_len:
                            continue
                        pts = np.array([[c, r] for r, c in seg], dtype=np.float32).reshape(-1, 1, 2)
                        epsilon = max(1, int(2 * scale))
                        approx = cv2.approxPolyDP(pts, epsilon, False)
                        wpts = []
                        for pt in approx:
                            px_, py_ = int(pt[0][0]), int(pt[0][1])
                            wx, wz = px_to_world(px_, py_, bounds, wh, ww)
                            wpts.append({"x": round(wx, 2), "z": round(wz, 2)})
                        edges.append({"from": loop_nids[si], "to": loop_nids[si + 1],
                                      "type": rtype, "points": wpts})
                    continue

                avg_brightness = brightness_sum / len(path_px)
                rtype = road_type(avg_brightness)

                # 경로 단순화 (Douglas-Peucker)
                pts = np.array([[c, r] for r, c in path_px], dtype=np.float32).reshape(-1, 1, 2)
                epsilon = max(1, int(2 * scale))
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

    # ── 중복 엣지 제거 ─────────────────────────────────────────────────────────
    # 같은 (from,to) 또는 (to,from) 사이에 중복 엣지가 생길 수 있음 (두 차선 평행 스켈레톤)
    # 타입 우선순위: highway > major > local
    type_rank = {"highway": 2, "major": 1, "local": 0}
    seen_pairs = {}   # (min_id, max_id) → edge index
    deduped = []
    for e in edges:
        key = (min(e["from"], e["to"]), max(e["from"], e["to"]))
        if key not in seen_pairs:
            seen_pairs[key] = len(deduped)
            deduped.append(e)
        else:
            # 더 높은 타입의 엣지로 교체
            prev = deduped[seen_pairs[key]]
            if type_rank.get(e["type"], 0) > type_rank.get(prev["type"], 0):
                deduped[seen_pairs[key]] = e
    removed_dup = len(edges) - len(deduped)
    edges = deduped
    if removed_dup:
        print(f"      중복 엣지 제거: {removed_dup}개 제거  (남은 엣지: {len(edges)})")

    # ── 장거리 엣지 highway 승격 ────────────────────────────────────────────────
    # 100 월드 단위 이상을 잇는 엣지는 하이웨이 링 구간으로 판단, highway 로 분류
    HWY_SPAN = 100.0
    promoted = 0
    for e in edges:
        pts = e.get("points", [])
        if not pts:
            continue
        xs_ = [p["x"] for p in pts]
        zs_ = [p["z"] for p in pts]
        span = max(max(xs_) - min(xs_), max(zs_) - min(zs_))
        if span >= HWY_SPAN and e["type"] != "highway":
            e["type"] = "highway"
            promoted += 1
    if promoted:
        print(f"      장거리 엣지 highway 승격: {promoted}개")

    # ── 긴 엣지 분할 (하이웨이 링 라우팅 지원) ──────────────────────────────────
    # highway 엣지가 너무 길면 A*가 링 위의 중간 지점을 찾지 못함.
    # 100 월드 단위마다 중간 노드를 삽입해 엣지를 분할한다.
    SPLIT_EVERY = 100.0
    split_edges_new = []
    split_removed = 0
    for e in edges:
        if e.get("type") != "highway":
            split_edges_new.append(e)
            continue
        pts = e["points"]
        # 폴리라인 총 길이 계산
        segs_len = []
        total_len = 0.0
        for i in range(1, len(pts)):
            dx = pts[i]["x"] - pts[i-1]["x"]
            dz = pts[i]["z"] - pts[i-1]["z"]
            l = (dx*dx + dz*dz) ** 0.5
            segs_len.append(l)
            total_len += l
        n_segs = max(1, int(total_len / SPLIT_EVERY))
        if n_segs <= 1:
            split_edges_new.append(e)
            continue
        # 균등 분할: 누적 길이 기준으로 분할점 찾기
        seg_len_target = total_len / n_segs
        cur_from = e["from"]
        cur_pts = [pts[0]]
        accumulated = 0.0
        seg_idx = 0
        for i, seg_l in enumerate(segs_len):
            accumulated += seg_l
            cur_pts.append(pts[i+1])
            if seg_idx < n_segs - 1 and accumulated >= seg_len_target * (seg_idx + 1):
                # 분할점: 새 노드 삽입
                new_nid = len(nodes)
                p = pts[i+1]
                nodes.append({"id": new_nid, "x": p["x"], "z": p["z"]})
                split_edges_new.append({"from": cur_from, "to": new_nid,
                                        "type": "highway", "points": cur_pts[:]})
                cur_from = new_nid
                cur_pts = [p]
                seg_idx += 1
        # 마지막 세그먼트
        cur_pts.append(pts[-1]) if cur_pts[-1] != pts[-1] else None
        split_edges_new.append({"from": cur_from, "to": e["to"],
                                 "type": "highway", "points": cur_pts})
        split_removed += 1
    if split_removed:
        added_nodes = len(nodes) - len(new_nodes if 'new_nodes' in dir() else nodes)
        print(f"      하이웨이 엣지 분할: {split_removed}개 엣지 분할, "
              f"노드 {len(nodes) - 28}개 추가")  # approximate
        edges = split_edges_new
    else:
        edges = split_edges_new

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
    ap.add_argument("--threshold", type=int, default=155,
                    help="도로 이진화 임계값 0-255 (기본 155: 실제 도로만 포함, 광장/주차장 제외)\n"
                         "  155 = highway/4tracks/2tracks 포함, asphalt_square/extra 제외\n"
                         "  40  = 모든 아스팔트 포함 (이전 동작)")
    args = ap.parse_args()

    bounds = {"min_x": args.min_x, "max_x": args.max_x,
              "min_z": args.min_z, "max_z": args.max_z}

    nodes, edges = extract_graph(args.mask, bounds, args.work_size, threshold=args.threshold)

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
