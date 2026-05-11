#ifndef ENTERTAINMENT_ROADGRAPH_HPP
#define ENTERTAINMENT_ROADGRAPH_HPP

#include <QVector>
#include <QPointF>
#include <QString>
#include <QHash>

/// 도로 엣지 타입
enum class RoadType { Local, Major, Highway };

/// 도로 그래프 노드
struct RGNode {
    int    id;
    double x, z;
};

/// 도로 그래프 엣지
struct RGEdge {
    int      from, to;
    RoadType type;
    QVector<QPointF> points;   // 월드 XZ 좌표 폴리라인 (첫/끝이 from/to 노드)
    double   length;           // 유클리드 근사 길이
};

/// road_graph.json 로더 + A* 경로 탐색
class RoadGraph {
public:
    bool load(const QString &jsonPath);
    bool isLoaded() const { return loaded_; }

    const QVector<RGNode> &nodes() const { return nodes_; }
    const QVector<RGEdge> &edges() const { return edges_; }

    /// 월드 좌표 (wx, wz) → 가장 가까운 그래프 노드 ID
    int nearestNode(double wx, double wz) const;

    /// A* 경로 탐색 → 월드 XZ 좌표 시퀀스 (없으면 빈 벡터)
    QVector<QPointF> findPath(int fromNode, int toNode) const;

    /// 편의: 월드 좌표로 바로 경로 탐색
    QVector<QPointF> findPath(double fromX, double fromZ,
                              double toX,   double toZ) const;

private:
    bool loaded_{false};
    QVector<RGNode> nodes_;
    QVector<RGEdge> edges_;

    /// 인접 리스트 (node id → edge indices)
    QHash<int, QVector<int>> adj_;
};

#endif // ENTERTAINMENT_ROADGRAPH_HPP
