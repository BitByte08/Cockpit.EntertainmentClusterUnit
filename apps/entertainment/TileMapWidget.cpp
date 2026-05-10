#include "TileMapWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QCoreApplication>
#include <QtMath>
#include <QDir>

TileMapWidget::TileMapWidget(QWidget *parent) : QWidget(parent) {
    // 기본 타일 경로: 실행파일 옆 tiles/ 폴더
    tile_path_ = QCoreApplication::applicationDirPath() + "/tiles";
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMinimumSize(200, 150);
}

void TileMapWidget::setTilePath(const QString &path) {
    tile_path_ = path;
    tile_cache_.clear();
    update();
}

void TileMapWidget::setWorldBounds(double minX, double maxX, double minZ, double maxZ) {
    world_min_x_ = minX; world_max_x_ = maxX;
    world_min_z_ = minZ; world_max_z_ = maxZ;
    tile_cache_.clear();
    update();
}

void TileMapWidget::setZoom(int zoom) {
    zoom_ = qBound(0, zoom, 6);
    tile_cache_.clear();
    update();
}

void TileMapWidget::setPosition(double worldX, double worldZ) {
    pos_x_ = worldX;
    pos_z_ = worldZ;
    update();
}

void TileMapWidget::setHeading(double degrees) {
    heading_ = degrees;
    update();
}

void TileMapWidget::wheelEvent(QWheelEvent *e) {
    if (e->angleDelta().y() > 0) setZoom(zoom_ + 1);
    else                          setZoom(zoom_ - 1);
}

// ── 좌표 변환 ────────────────────────────────────────────────────────────────
// 월드 좌표 → 전체 맵 픽셀 좌표 (tileY=0이 maxZ = 북쪽)
void TileMapWidget::worldToMap(double wx, double wz, double &mx, double &my) const {
    double totalPx = static_cast<double>(kTileSize) * (1 << zoom_);
    double worldW  = world_max_x_ - world_min_x_;
    double worldH  = world_max_z_ - world_min_z_;
    mx = (wx - world_min_x_) / worldW * totalPx;
    my = (world_max_z_ - wz) / worldH * totalPx;  // Z 반전
}

// ── 타일 로딩 ────────────────────────────────────────────────────────────────
QPixmap TileMapWidget::loadTile(int z, int tx, int ty) {
    QString key = QStringLiteral("%1/%2/%3").arg(z).arg(tx).arg(ty);

    if (tile_cache_.contains(key))
        return tile_cache_.value(key);

    // 캐시 크기 제한
    if (tile_cache_.size() >= kCacheMax)
        tile_cache_.clear();

    QString path = QStringLiteral("%1/%2/%3/%4.png")
                       .arg(tile_path_).arg(z).arg(tx).arg(ty);
    QPixmap pm(path);

    // 타일 없으면 격자 플레이스홀더 생성
    if (pm.isNull()) {
        pm = QPixmap(kTileSize, kTileSize);
        pm.fill(QColor(0x10, 0x10, 0x18));
        QPainter p(&pm);
        p.setPen(QColor(0x22, 0x22, 0x33));
        p.drawRect(0, 0, kTileSize - 1, kTileSize - 1);
        p.setPen(QColor(0x1C, 0x69, 0xD4, 80));
        p.setFont(QFont("Monospace", 7));
        p.drawText(pm.rect(), Qt::AlignCenter,
                   QStringLiteral("%1/%2/%3").arg(z).arg(tx).arg(ty));
    }

    tile_cache_.insert(key, pm);
    return pm;
}

// ── 페인트 ───────────────────────────────────────────────────────────────────
void TileMapWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int W = width(), H = height();

    // 배경
    p.fillRect(rect(), QColor(0x08, 0x08, 0x10));

    // 차량 위치 → 맵 픽셀 좌표
    double vMx, vMy;
    worldToMap(pos_x_, pos_z_, vMx, vMy);

    // 뷰포트: 차량 중앙 기준
    double viewOffX = vMx - W / 2.0;
    double viewOffY = vMy - H / 2.0;

    int tileCount = 1 << zoom_;
    double totalPx = static_cast<double>(kTileSize) * tileCount;

    // 화면에 보여야 할 타일 범위 계산
    int txMin = static_cast<int>(qFloor(viewOffX / kTileSize));
    int tyMin = static_cast<int>(qFloor(viewOffY / kTileSize));
    int txMax = static_cast<int>(qFloor((viewOffX + W - 1) / kTileSize));
    int tyMax = static_cast<int>(qFloor((viewOffY + H - 1) / kTileSize));

    // 타일 그리기
    for (int ty = tyMin; ty <= tyMax; ty++) {
        for (int tx = txMin; tx <= txMax; tx++) {
            // 맵 범위 클리핑
            if (tx < 0 || ty < 0 || tx >= tileCount || ty >= tileCount)
                continue;

            QPixmap tile = loadTile(zoom_, tx, ty);
            int drawX = static_cast<int>(tx * kTileSize - viewOffX);
            int drawY = static_cast<int>(ty * kTileSize - viewOffY);
            p.drawPixmap(drawX, drawY, tile);
        }
    }

    // ── 맵 경계 외부: 어두운 오버레이 ───────────────────────────────────────
    {
        int mapL = static_cast<int>(-viewOffX);
        int mapT = static_cast<int>(-viewOffY);
        int mapR = static_cast<int>(totalPx - viewOffX);
        int mapB = static_cast<int>(totalPx - viewOffY);
        p.fillRect(0, 0, mapL, H, QColor(0x00, 0x00, 0x00, 180));
        p.fillRect(mapR, 0, W - mapR, H, QColor(0x00, 0x00, 0x00, 180));
        p.fillRect(0, 0, W, mapT, QColor(0x00, 0x00, 0x00, 180));
        p.fillRect(0, mapB, W, H - mapB, QColor(0x00, 0x00, 0x00, 180));
    }

    // ── 차량 마커 (화면 정중앙) ──────────────────────────────────────────────
    {
        const int cx = W / 2;
        const int cy = H / 2;

        p.save();
        p.translate(cx, cy);
        p.rotate(heading_);   // 0=North=위, 90=East=오른쪽

        // 외곽 원 (BMW M blue 글로우)
        p.setBrush(QColor(0x1C, 0x69, 0xD4, 50));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPoint(0, 0), 14, 14);

        // 방향 삼각형 (위쪽이 진행 방향)
        QPainterPath arrow;
        arrow.moveTo(0, -13);      // 앞 꼭지점
        arrow.lineTo(-7,  6);      // 좌측 하단
        arrow.lineTo( 0,  2);      // 중앙 오목
        arrow.lineTo( 7,  6);      // 우측 하단
        arrow.closeSubpath();

        p.setBrush(QColor(0x1C, 0x69, 0xD4));
        p.setPen(QPen(Qt::white, 1.0));
        p.drawPath(arrow);

        p.restore();
    }

    // ── 줌 레벨 표시 (우하단) ────────────────────────────────────────────────
    {
        QFont f;
        f.setPointSize(8);
        p.setFont(f);
        p.setPen(QColor(0x44, 0x44, 0x55));
        p.drawText(QRect(W - 50, H - 18, 46, 14), Qt::AlignRight | Qt::AlignVCenter,
                   QStringLiteral("z%1").arg(zoom_));
    }
}
