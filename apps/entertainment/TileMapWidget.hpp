#ifndef ENTERTAINMENT_TILEMAPWIDGET_HPP
#define ENTERTAINMENT_TILEMAPWIDGET_HPP

#include <QWidget>
#include <QPixmap>
#include <QMap>
#include <QString>
#include <QVector>
#include <QPointF>
#include "RoadGraph.hpp"

/// 슬리피맵 타일 + 벡터 도로 네비게이션 렌더러 (BMW M 스타일)
///
/// MapMode::Satellite  — 타일 이미지 표시 (기존)
/// MapMode::Navigation — 벡터 도로 표시 (현대 네비 스타일)
///
/// 타일 포맷: {tilePath}/{zoom}/{tileX}/{tileY}.png  (1024×1024 PNG)
/// 좌표 규약:
///   tileX=0, tileY=0 → 월드 북서쪽 (minX, maxZ)
///   tileX 증가 = East (+X)  /  tileY 증가 = South (-Z)
class TileMapWidget : public QWidget {
    Q_OBJECT
public:
    enum class MapMode { Satellite, Navigation };

    explicit TileMapWidget(QWidget *parent = nullptr);

    void setTilePath(const QString &path);
    void setWorldBounds(double minX, double maxX, double minZ, double maxZ);
    void setZoom(int zoom);
    int  zoom()     const { return zoom_; }
    bool hasDestination() const { return has_dest_; }

    /// road_graph.json 로드 → Navigation 모드 자동 활성화
    bool loadRoadGraph(const QString &jsonPath);

    void setMapMode(MapMode mode);
    MapMode mapMode() const { return map_mode_; }

signals:
    void destinationChanged(double wx, double wz);
    void destinationCleared();
    void distanceToDestChanged(double meters);

public slots:
    void setPosition(double worldX, double worldZ);
    void setHeading(double degrees);
    void setSpeedKmh(double kmh);
    void setDestination(double wx, double wz);
    void clearDestination();

    QSize sizeHint()        const override { return QSize(800, 400); }
    QSize minimumSizeHint() const override { return QSize(400, 200); }

protected:
    void paintEvent(QPaintEvent *) override;
    void wheelEvent(QWheelEvent *)  override;
    void mousePressEvent(QMouseEvent *) override;

private:
    // 타일 모드
    QPixmap loadTile(int z, int tx, int ty);
    void    paintSatellite(QPainter &p, int W, int H,
                           double viewOffX, double viewOffY);

    // 네비 모드
    void paintNavigation(QPainter &p, int W, int H,
                         double viewOffX, double viewOffY);
    void paintRoadEdge(QPainter &p, const RGEdge &edge,
                       double viewOffX, double viewOffY);

    // 공통
    void worldToMap(double wx, double wz, double &mx, double &my) const;
    void recalcDistance();
    void recalcRoute();
    static int speedToZoom(double kmh);

    // ── 멤버 ──────────────────────────────────────────────────────────────
    MapMode  map_mode_{MapMode::Satellite};
    RoadGraph road_graph_;

    QString  tile_path_;
    int      zoom_{2};

    double world_min_x_{-400.0}, world_max_x_{450.0};
    double world_min_z_{-200.0}, world_max_z_{240.0};

    double pos_x_{0.0}, pos_z_{0.0};
    double heading_{0.0};

    bool   auto_zoom_{true};

    bool   has_dest_{false};
    double dest_x_{0.0}, dest_z_{0.0};
    QVector<QPointF> route_;    // A* 경로 (네비 모드)

    static constexpr int kTileSize = 1024;
    static constexpr int kCacheMax = 64;
    QMap<QString, QPixmap> tile_cache_;
};

#endif // ENTERTAINMENT_TILEMAPWIDGET_HPP
