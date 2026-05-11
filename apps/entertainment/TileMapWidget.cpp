#include "TileMapWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QCoreApplication>
#include <QtMath>
#include <QDir>
#include <cmath>

TileMapWidget::TileMapWidget(QWidget *parent) : QWidget(parent) {
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
    auto_zoom_ = false;   // 수동 조작 시 auto-zoom 해제
    zoom_ = qBound(0, zoom, 6);
    tile_cache_.clear();
    update();
}

void TileMapWidget::setPosition(double worldX, double worldZ) {
    pos_x_ = worldX;
    pos_z_ = worldZ;
    recalcDistance();
    update();
}

void TileMapWidget::setHeading(double degrees) {
    heading_ = degrees;
    update();
}

// ── auto-zoom ────────────────────────────────────────────────────────────────

int TileMapWidget::speedToZoom(double kmh) {
    if (kmh <= 5)   return 6;
    if (kmh <= 20)  return 5;
    if (kmh <= 50)  return 4;
    if (kmh <= 100) return 3;
    return 2;
}

void TileMapWidget::setSpeedKmh(double kmh) {
    if (!auto_zoom_) return;
    int target = speedToZoom(kmh);
    if (target == zoom_) return;
    zoom_ = target;
    tile_cache_.clear();
    update();
}

// ── 경로 ─────────────────────────────────────────────────────────────────────

void TileMapWidget::setDestination(double wx, double wz) {
    has_dest_ = true;
    dest_x_ = wx;
    dest_z_ = wz;
    recalcDistance();
    update();
}

void TileMapWidget::clearDestination() {
    has_dest_ = false;
    emit destinationCleared();
    emit distanceToDestChanged(-1.0);
    update();
}

void TileMapWidget::recalcDistance() {
    if (!has_dest_) return;
    double dx = dest_x_ - pos_x_;
    double dz = dest_z_ - pos_z_;
    emit distanceToDestChanged(std::sqrt(dx * dx + dz * dz));
}

// ── 마우스 클릭 → 목적지 설정 ─────────────────────────────────────────────────

void TileMapWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton) { QWidget::mousePressEvent(e); return; }

    // 현재 뷰포트 오프셋 계산 (paintEvent와 동일 로직)
    double vMx, vMy;
    worldToMap(pos_x_, pos_z_, vMx, vMy);
    double viewOffX = vMx - width()  / 2.0;
    double viewOffY = vMy - height() / 2.0;

    double totalPx = static_cast<double>(kTileSize) * (1 << zoom_);
    double worldW  = world_max_x_ - world_min_x_;
    double worldH  = world_max_z_ - world_min_z_;

    double mapPx = e->position().x() + viewOffX;
    double mapPy = e->position().y() + viewOffY;
    double wx = world_min_x_ + mapPx / totalPx * worldW;
    double wz = world_max_z_ - mapPy / totalPx * worldH;

    // 오른쪽 클릭 or 목적지 재클릭(±15px 이내) → 목적지 취소
    if (has_dest_) {
        double dsx = e->position().x() - (width() / 2.0 + (dest_x_ - pos_x_) / worldW * totalPx);
        double dsy = e->position().y() - (height() / 2.0 - (dest_z_ - pos_z_) / worldH * totalPx);
        // 위 계산 대신 단순히: dest 스크린 좌표
        double destMapX, destMapY;
        worldToMap(dest_x_, dest_z_, destMapX, destMapY);
        double destScrX = destMapX - viewOffX;
        double destScrY = destMapY - viewOffY;
        double d2 = std::sqrt(
            (e->position().x() - destScrX) * (e->position().x() - destScrX) +
            (e->position().y() - destScrY) * (e->position().y() - destScrY));
        if (d2 < 20.0) { clearDestination(); return; }
    }

    setDestination(wx, wz);
    emit destinationChanged(wx, wz);
}

// ── 좌표 변환 ────────────────────────────────────────────────────────────────

void TileMapWidget::worldToMap(double wx, double wz, double &mx, double &my) const {
    double totalPx = static_cast<double>(kTileSize) * (1 << zoom_);
    double worldW  = world_max_x_ - world_min_x_;
    double worldH  = world_max_z_ - world_min_z_;
    mx = (wx - world_min_x_) / worldW * totalPx;
    my = (world_max_z_ - wz)  / worldH * totalPx;
}

// ── 타일 로딩 ────────────────────────────────────────────────────────────────

QPixmap TileMapWidget::loadTile(int z, int tx, int ty) {
    QString key = QStringLiteral("%1/%2/%3").arg(z).arg(tx).arg(ty);
    if (tile_cache_.contains(key)) return tile_cache_.value(key);

    if (tile_cache_.size() >= kCacheMax) tile_cache_.clear();

    QString path = QStringLiteral("%1/%2/%3/%4.png")
                       .arg(tile_path_).arg(z).arg(tx).arg(ty);
    QPixmap pm(path);

    if (pm.isNull()) {
        pm = QPixmap(kTileSize, kTileSize);
        pm.fill(QColor(0x10, 0x10, 0x18));
        QPainter p(&pm);
        p.setPen(QColor(0x22, 0x22, 0x33));
        p.drawRect(0, 0, kTileSize - 1, kTileSize - 1);
        p.setPen(QColor(0x1C, 0x69, 0xD4, 80));
        p.setFont(QFont("Monospace", 40));
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
    p.fillRect(rect(), QColor(0x08, 0x08, 0x10));

    double vMx, vMy;
    worldToMap(pos_x_, pos_z_, vMx, vMy);
    double viewOffX = vMx - W / 2.0;
    double viewOffY = vMy - H / 2.0;

    int tileCount = 1 << zoom_;
    double totalPx = static_cast<double>(kTileSize) * tileCount;

    int txMin = static_cast<int>(qFloor(viewOffX / kTileSize));
    int tyMin = static_cast<int>(qFloor(viewOffY / kTileSize));
    int txMax = static_cast<int>(qFloor((viewOffX + W - 1) / kTileSize));
    int tyMax = static_cast<int>(qFloor((viewOffY + H - 1) / kTileSize));

    // 정수 기준점으로 타일 틈새 방지
    int originX = qRound(txMin * kTileSize - viewOffX);
    int originY = qRound(tyMin * kTileSize - viewOffY);

    for (int ty = tyMin; ty <= tyMax; ty++) {
        for (int tx = txMin; tx <= txMax; tx++) {
            if (tx < 0 || ty < 0 || tx >= tileCount || ty >= tileCount) continue;
            QPixmap tile = loadTile(zoom_, tx, ty);
            int drawX = originX + (tx - txMin) * kTileSize;
            int drawY = originY + (ty - tyMin) * kTileSize;
            p.drawPixmap(drawX, drawY, tile);
        }
    }

    // 맵 경계 외부 어두운 오버레이
    {
        int mapL = qRound(-viewOffX);
        int mapT = qRound(-viewOffY);
        int mapR = qRound(totalPx - viewOffX);
        int mapB = qRound(totalPx - viewOffY);
        QColor ov(0, 0, 0, 180);
        if (mapL > 0)  p.fillRect(0,    0, mapL,    H, ov);
        if (mapR < W)  p.fillRect(mapR, 0, W - mapR, H, ov);
        if (mapT > 0)  p.fillRect(0,    0, W, mapT,    ov);
        if (mapB < H)  p.fillRect(0, mapB, W, H - mapB, ov);
    }

    // ── 경로선 (목적지까지 파란 점선) ────────────────────────────────────────
    if (has_dest_) {
        double destMapX, destMapY;
        worldToMap(dest_x_, dest_z_, destMapX, destMapY);
        double destScrX = destMapX - viewOffX;
        double destScrY = destMapY - viewOffY;

        // 파란 점선
        QPen routePen(QColor(0x1C, 0x69, 0xD4, 200), 2.5, Qt::DashLine);
        routePen.setDashPattern({6, 5});
        p.setPen(routePen);
        p.drawLine(QPointF(W / 2.0, H / 2.0), QPointF(destScrX, destScrY));

        // 목적지 핀 마커
        p.save();
        p.translate(destScrX, destScrY);
        // 외곽 원
        p.setBrush(QColor(0x1C, 0x69, 0xD4));
        p.setPen(QPen(Qt::white, 2));
        p.drawEllipse(QPoint(0, 0), 8, 8);
        // 내부 흰 점
        p.setBrush(Qt::white);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPoint(0, 0), 3, 3);
        p.restore();
    }

    // ── 차량 마커 (화면 정중앙) ──────────────────────────────────────────────
    {
        const int cx = W / 2;
        const int cy = H / 2;

        p.save();
        p.translate(cx, cy);
        p.rotate(heading_);

        p.setBrush(QColor(0x1C, 0x69, 0xD4, 50));
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPoint(0, 0), 14, 14);

        QPainterPath arrow;
        arrow.moveTo(0, -13);
        arrow.lineTo(-7,  6);
        arrow.lineTo( 0,  2);
        arrow.lineTo( 7,  6);
        arrow.closeSubpath();

        p.setBrush(QColor(0x1C, 0x69, 0xD4));
        p.setPen(QPen(Qt::white, 1.0));
        p.drawPath(arrow);

        p.restore();
    }

    // ── 줌 레벨 + auto 표시 ──────────────────────────────────────────────────
    {
        QFont f; f.setPointSize(8);
        p.setFont(f);
        p.setPen(QColor(0x44, 0x44, 0x55));
        QString zStr = auto_zoom_
            ? QStringLiteral("z%1 · AUTO").arg(zoom_)
            : QStringLiteral("z%1").arg(zoom_);
        p.drawText(QRect(W - 80, H - 18, 76, 14),
                   Qt::AlignRight | Qt::AlignVCenter, zStr);
    }
}

void TileMapWidget::wheelEvent(QWheelEvent *e) {
    auto_zoom_ = false;   // 스크롤 줌 시 auto-zoom 해제
    if (e->angleDelta().y() > 0) { zoom_ = qBound(0, zoom_ + 1, 6); }
    else                          { zoom_ = qBound(0, zoom_ - 1, 6); }
    tile_cache_.clear();
    update();
}
