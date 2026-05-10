#ifndef ENTERTAINMENT_TILEMAPWIDGET_HPP
#define ENTERTAINMENT_TILEMAPWIDGET_HPP

#include <QWidget>
#include <QPixmap>
#include <QMap>
#include <QString>

/// 슬리피맵 타일 기반 지도 렌더러 (BMW M 스타일)
///
/// 타일 포맷: {tilePath}/{zoom}/{tileX}/{tileY}.png  (256×256 PNG)
/// 좌표 규약:
///   tileX=0, tileY=0 → 월드 북서쪽 (minX, maxZ)
///   tileX 증가 = East (+X 방향)
///   tileY 증가 = South (-Z 방향)
///
/// 차량 마커: heading=0 → 북쪽(위), 90 → 동쪽(오른쪽)
class TileMapWidget : public QWidget {
    Q_OBJECT
public:
    explicit TileMapWidget(QWidget *parent = nullptr);

    void setTilePath(const QString &path);     // 타일 루트 디렉토리
    void setWorldBounds(double minX, double maxX, double minZ, double maxZ);
    void setZoom(int zoom);                    // 0~6
    int  zoom() const { return zoom_; }

public slots:
    void setPosition(double worldX, double worldZ);
    void setHeading(double degrees);           // 0=North, 90=East

    QSize sizeHint()        const override { return QSize(800, 400); }
    QSize minimumSizeHint() const override { return QSize(400, 200); }

protected:
    void paintEvent(QPaintEvent *) override;
    void wheelEvent(QWheelEvent *) override;

private:
    QPixmap loadTile(int z, int tx, int ty);
    void    worldToMap(double wx, double wz, double &mx, double &my) const;

    QString tile_path_;
    int     zoom_{3};

    double world_min_x_{-500.0}, world_max_x_{500.0};
    double world_min_z_{-500.0}, world_max_z_{500.0};

    double pos_x_{0.0}, pos_z_{0.0};
    double heading_{0.0};

    static constexpr int kTileSize = 256;
    static constexpr int kCacheMax = 256;

    QMap<QString, QPixmap> tile_cache_;
};

#endif // ENTERTAINMENT_TILEMAPWIDGET_HPP
