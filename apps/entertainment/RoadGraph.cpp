#include "RoadGraph.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <cmath>
#include <queue>
#include <vector>
#include <unordered_map>
#include <limits>

static double dist2(double ax, double az, double bx, double bz) {
    double dx = ax - bx, dz = az - bz;
    return dx * dx + dz * dz;
}

// ── 로더 ─────────────────────────────────────────────────────────────────────

bool RoadGraph::load(const QString &jsonPath) {
    loaded_ = false;
    nodes_.clear(); edges_.clear(); adj_.clear();

    QFile f(jsonPath);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "[RoadGraph] 파일 없음:" << jsonPath;
        return false;
    }

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "[RoadGraph] JSON 파싱 오류:" << err.errorString();
        return false;
    }

    auto root    = doc.object();
    auto jNodes  = root["nodes"].toArray();
    auto jEdges  = root["edges"].toArray();

    nodes_.reserve(jNodes.size());
    for (const auto &jn : jNodes) {
        auto o = jn.toObject();
        nodes_.push_back({o["id"].toInt(), o["x"].toDouble(), o["z"].toDouble()});
    }

    edges_.reserve(jEdges.size());
    for (const auto &je : jEdges) {
        auto o = je.toObject();
        RGEdge e;
        e.from = o["from"].toInt();
        e.to   = o["to"].toInt();

        QString t = o["type"].toString();
        if      (t == "highway") e.type = RoadType::Highway;
        else if (t == "major")   e.type = RoadType::Major;
        else                     e.type = RoadType::Local;

        auto jPts = o["points"].toArray();
        e.points.reserve(jPts.size());
        for (const auto &jp : jPts) {
            auto p = jp.toObject();
            e.points.append({p["x"].toDouble(), p["z"].toDouble()});
        }

        // edge 끝점을 정확한 노드 좌표로 스냅
        // → paintRoadEdge와 findPath가 동일 기준점 사용, 도로-경로 정렬
        if (!e.points.isEmpty()) {
            if (e.from >= 0 && e.from < (int)nodes_.size())
                e.points.first() = {nodes_[e.from].x, nodes_[e.from].z};
            if (e.to >= 0 && e.to < (int)nodes_.size())
                e.points.last()  = {nodes_[e.to].x,   nodes_[e.to].z};
        }

        // 폴리라인 총 길이 계산
        e.length = 0.0;
        for (int i = 1; i < e.points.size(); ++i) {
            double dx = e.points[i].x() - e.points[i-1].x();
            double dz = e.points[i].y() - e.points[i-1].y();
            e.length += std::sqrt(dx*dx + dz*dz);
        }

        int idx = edges_.size();
        edges_.push_back(e);
        adj_[e.from].append(idx);
        adj_[e.to].append(idx);   // 양방향
    }

    loaded_ = true;
    qDebug() << "[RoadGraph] 로드 완료: 노드" << nodes_.size()
             << "엣지" << edges_.size();
    return true;
}

// ── 가장 가까운 노드 ──────────────────────────────────────────────────────────

int RoadGraph::nearestNode(double wx, double wz) const {
    int   best = -1;
    double bestD2 = std::numeric_limits<double>::max();
    for (const auto &n : nodes_) {
        double d2 = dist2(wx, wz, n.x, n.z);
        if (d2 < bestD2) { bestD2 = d2; best = n.id; }
    }
    return best;
}

// ── A* 경로 탐색 ──────────────────────────────────────────────────────────────

QVector<QPointF> RoadGraph::findPath(int fromNode, int toNode) const {
    if (!loaded_ || fromNode < 0 || toNode < 0 || fromNode >= nodes_.size() || toNode >= nodes_.size())
        return {};
    if (fromNode == toNode) return {{ nodes_[fromNode].x, nodes_[fromNode].z }};

    const int N = nodes_.size();
    std::vector<double> gScore(N, std::numeric_limits<double>::max());
    std::vector<int>    cameEdge(N, -1);
    std::vector<bool>   cameReverse(N, false);

    auto h = [&](int nid) {
        return std::sqrt(dist2(nodes_[nid].x, nodes_[nid].z,
                               nodes_[toNode].x, nodes_[toNode].z));
    };

    using PD = std::pair<double, int>;
    std::priority_queue<PD, std::vector<PD>, std::greater<PD>> open;

    gScore[fromNode] = 0.0;
    open.push({h(fromNode), fromNode});

    while (!open.empty()) {
        auto [f, cur] = open.top(); open.pop();

        if (cur == toNode) break;
        if (f > gScore[cur] + h(cur) + 1e-9) continue;  // stale entry

        for (int eidx : adj_.value(cur)) {
            const RGEdge &e = edges_[eidx];
            int nb = (e.from == cur) ? e.to : e.from;
            double newG = gScore[cur] + e.length;
            if (newG < gScore[nb]) {
                gScore[nb]      = newG;
                cameEdge[nb]    = eidx;
                cameReverse[nb] = (e.to == cur);   // 역방향 진입이면 true
                open.push({newG + h(nb), nb});
            }
        }
    }

    if (gScore[toNode] == std::numeric_limits<double>::max())
        return {};   // 경로 없음

    // ── 역추적 ─────────────────────────────────────────────────────────────
    // edge endpoints가 이미 노드 좌표로 스냅돼 있으므로
    // 접합점 중복 없이 그대로 이어 붙이면 됨
    QVector<QPointF> path;
    int cur = toNode;
    while (cur != fromNode) {
        int eidx = cameEdge[cur];
        if (eidx < 0) break;
        const RGEdge &e = edges_[eidx];
        QVector<QPointF> seg = e.points;
        if (cameReverse[cur]) {
            for (int i = 0, j = seg.size()-1; i < j; ++i, --j)
                std::swap(seg[i], seg[j]);
        }
        if (!path.isEmpty()) seg.removeLast();   // 뒤쪽 노드 좌표는 이미 path 앞에 있음
        path = seg + path;
        cur = cameReverse[cur] ? e.to : e.from;
    }
    return path;
}

QVector<QPointF> RoadGraph::findPath(double fromX, double fromZ,
                                     double toX,   double toZ) const {
    return findPath(nearestNode(fromX, fromZ), nearestNode(toX, toZ));
}
