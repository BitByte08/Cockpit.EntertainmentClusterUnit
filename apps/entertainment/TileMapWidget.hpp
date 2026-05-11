#ifndef ENTERTAINMENT_TILEMAPWIDGET_HPP
#define ENTERTAINMENT_TILEMAPWIDGET_HPP

#include <QWidget>
#include <QPixmap>
#include <QMap>
#include <QString>

/// 슬리피맵 타일 기반 지도 렌더러 (BMW M 스타일)
///
/// 타일 포맷: {tilePath}/{zoom}/{tileX}/{tileY}.png  (1024×1024 PNG)
/// 좌표 규약:
///   tileX=0, tileY=0 → 월드 북서쪽 (minX, maxZ)
///   tileX 증가 = East (+X 방향)
///   tileY 증가 = South (-Z 방향)
///
/// 차량 마커: heading=0 → 북쪽(위), 90 → 동쪽(오른쪽)
/// 자동줌:   속도에 따라 자동으로 줌 레벨 조정
/// 경로:     지도 클릭 시 목적지 설정, 파란 점선 경로 표시
class TileMapWidget : public QWidget {
    Q_OBJECT
public:
    explicit TileMapWidget(QWidget *parent = nullptr);

    void setTilePath(const QString &path);
    void setWorldBounds(double minX, double maxX, double minZ, double maxZ);
    void setZoom(int zoom);     // 수동 줌 (auto-zoom 비활성화)
    int  zoom() const { return zoom_; }

    bool hasDestination() const { return has_dest_; }

signals:
    void destinationChanged(double wx, double wz);   // 지도 클릭 시
    void destinationCleared();
    void distanceToDestChanged(double meters);        // 위치/목적지 변경 시

public slots:
    void setPosition(double worldX, double worldZ);
    void setHeading(double degrees);
    void setSpeedKmh(double kmh);       // auto-zoom 용
    void setDestination(double wx, double wz);
    void clearDestination();

    QSize sizeHint()        const override { return QSize(800, 400); }
    QSize minimumSizeHint() const override { return QSize(400, 200); }

protected:
    void paintEvent(QPaintEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void mousePressEvent(QMouseEvent *) override;

private:
    QPixmap loadTile(int z, int tx, int ty);
    void    worldToMap(double wx, double wz, double &mx, double &my) const;
    void    recalcDistance();
    static int speedToZoom(double kmh);

    QString tile_path_;
    int     zoom_{2};

    double world_min_x_{-400.0}, world_max_x_{450.0};
    double world_min_z_{-200.0}, world_max_z_{240.0};

    double pos_x_{0.0}, pos_z_{0.0};
    double heading_{0.0};

    // auto-zoom
    bool   auto_zoom_{true};

    // route
    bool   has_dest_{false};
    double dest_x_{0.0}, dest_z_{0.0};

    static constexpr int kTileSize = 1024;
    static constexpr int kCacheMax = 64;

    QMap<QString, QPixmap> tile_cache_;
};

#endif // ENTERTAINMENT_TILEMAPWIDGET_HPP
