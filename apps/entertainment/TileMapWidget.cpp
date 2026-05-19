#include "TileMapWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QCoreApplication>
#include <QtMath>
#include <cmath>

// BMW M 색상 팔레트
static const QColor kNavBg      {0x0a, 0x0a, 0x12};   // 배경
static const QColor kRoadLocal  {0x35, 0x35, 0x45};   // 일반 도로
static const QColor kRoadMajor  {0x48, 0x48, 0x60};   // 주요 도로
static const QColor kRoadHwy    {0x60, 0x60, 0x80};   // 고속도로
static const QColor kMBlue      {0x1C, 0x69, 0xD4};   // M 블루
static const QColor kMBlueD     {0x0e, 0x44, 0x8a};   // M 블루 다크 (경계)

// ── 초기화 ────────────────────────────────────────────────────────────────────

TileMapWidget::TileMapWidget(QWidget *parent) : QWidget(parent) {
    tile_path_ = QCoreApplication::applicationDirPath() + "/tiles";
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMinimumSize(200, 150);
}

// ── 설정 ─────────────────────────────────────────────────────────────────────

void TileMapWidget::setTilePath(const QString &path) {
    tile_path_ = path; tile_cache_.clear(); update();
}

void TileMapWidget::setWorldBounds(double minX, double maxX, double minZ, double maxZ) {
    world_min_x_ = minX; world_max_x_ = maxX;
    world_min_z_ = minZ; world_max_z_ = maxZ;
    tile_cache_.clear(); update();
}

void TileMapWidget::setZoom(int zoom) {
    auto_zoom_ = false;
    zoom_ = qBound(0, zoom, 6);
    tile_cache_.clear(); update();
}

void TileMapWidget::setMapMode(MapMode mode) {
    map_mode_ = mode; update();
}

bool TileMapWidget::loadRoadGraph(const QString &jsonPath) {
    bool ok = road_graph_.load(jsonPath);
    if (ok) {
        map_mode_ = MapMode::Navigation;
        recalcRoute();
        update();
    }
    return ok;
}

// ── 위치/속도 슬롯 ────────────────────────────────────────────────────────────

void TileMapWidget::setPosition(double worldX, double worldZ) {
    pos_x_ = worldX; pos_z_ = worldZ;
    recalcDistance();
    if (map_mode_ == MapMode::Navigation && has_dest_) recalcRoute();
    update();
}

void TileMapWidget::setHeading(double degrees) {
    heading_ = degrees; update();
}

int TileMapWidget::speedToZoom(double kmh) {
    if (kmh <= 5)   return 6;
    if (kmh <= 20)  return 5;
    if (kmh <= 50)  return 4;
    if (kmh <= 100) return 3;
    return 2;
}

void TileMapWidget::setSpeedKmh(double kmh) {
    if (!auto_zoom_) return;
    int t = speedToZoom(kmh);
    if (t == zoom_) return;
    zoom_ = t; tile_cache_.clear(); update();
}

// ── 목적지 / 경로 ─────────────────────────────────────────────────────────────

void TileMapWidget::setDestination(double wx, double wz) {
    has_dest_ = true; dest_x_ = wx; dest_z_ = wz;
    recalcDistance();
    recalcRoute();
    update();
}

void TileMapWidget::clearDestination() {
    has_dest_ = false; route_.clear();
    emit destinationCleared();
    emit distanceToDestChanged(-1.0);
    update();
}

void TileMapWidget::recalcDistance() {
    if (!has_dest_) return;
    double dx = dest_x_ - pos_x_, dz = dest_z_ - pos_z_;
    emit distanceToDestChanged(std::sqrt(dx*dx + dz*dz));
}

void TileMapWidget::recalcRoute() {
    if (!has_dest_ || !road_graph_.isLoaded()) { route_.clear(); return; }
    route_ = road_graph_.findPath(pos_x_, pos_z_, dest_x_, dest_z_);
}

// ── 마우스 → 목적지 ────────────────────────────────────────────────────────────

void TileMapWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton) { QWidget::mousePressEvent(e); return; }

    double vMx, vMy;
    worldToMap(pos_x_, pos_z_, vMx, vMy);
    double viewOffX = vMx - width()  / 2.0;
    double viewOffY = vMy - height() / 2.0;
    double totalPx  = static_cast<double>(kTileSize) * (1 << zoom_);
    double worldW   = world_max_x_ - world_min_x_;
    double worldH   = world_max_z_ - world_min_z_;

    double wx = world_min_x_ + (e->position().x() + viewOffX) / totalPx * worldW;
    double wz = world_max_z_ - (e->position().y() + viewOffY) / totalPx * worldH;

    // 목적지 핀 근처 클릭 → 경로 취소
    if (has_dest_) {
        double destMapX, destMapY;
        worldToMap(dest_x_, dest_z_, destMapX, destMapY);
        double dsx = destMapX - viewOffX - e->position().x();
        double dsy = destMapY - viewOffY - e->position().y();
        if (std::sqrt(dsx*dsx + dsy*dsy) < 20.0) { clearDestination(); return; }
    }

    setDestination(wx, wz);
    emit destinationChanged(wx, wz);
}

// ── 스크롤 줌 ────────────────────────────────────────────────────────────────

void TileMapWidget::wheelEvent(QWheelEvent *e) {
    auto_zoom_ = false;
    zoom_ = qBound(0, zoom_ + (e->angleDelta().y() > 0 ? 1 : -1), 6);
    tile_cache_.clear(); update();
}

// ── 좌표 변환 ────────────────────────────────────────────────────────────────

void TileMapWidget::worldToMap(double wx, double wz, double &mx, double &my) const {
    double totalPx = static_cast<double>(kTileSize) * (1 << zoom_);
    mx = (wx - world_min_x_) / (world_max_x_ - world_min_x_) * totalPx;
    my = (world_max_z_ - wz)  / (world_max_z_ - world_min_z_) * totalPx;
}

// ── 타일 로딩 ────────────────────────────────────────────────────────────────

QPixmap TileMapWidget::loadTile(int z, int tx, int ty) {
    QString key = QStringLiteral("%1/%2/%3").arg(z).arg(tx).arg(ty);
    if (tile_cache_.contains(key)) return tile_cache_.value(key);
    if (tile_cache_.size() >= kCacheMax) tile_cache_.clear();

    QPixmap pm(QStringLiteral("%1/%2/%3/%4.png")
                   .arg(tile_path_).arg(z).arg(tx).arg(ty));
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

// ── 위성 모드 렌더링 ─────────────────────────────────────────────────────────

void TileMapWidget::paintSatellite(QPainter &p, int W, int H,
                                   double viewOffX, double viewOffY) {
    int tileCount = 1 << zoom_;
    double totalPx = static_cast<double>(kTileSize) * tileCount;

    int txMin = static_cast<int>(qFloor(viewOffX / kTileSize));
    int tyMin = static_cast<int>(qFloor(viewOffY / kTileSize));
    int txMax = static_cast<int>(qFloor((viewOffX + W - 1) / kTileSize));
    int tyMax = static_cast<int>(qFloor((viewOffY + H - 1) / kTileSize));

    int originX = qRound(txMin * kTileSize - viewOffX);
    int originY = qRound(tyMin * kTileSize - viewOffY);

    for (int ty = tyMin; ty <= tyMax; ty++) {
        for (int tx = txMin; tx <= txMax; tx++) {
            if (tx < 0 || ty < 0 || tx >= tileCount || ty >= tileCount) continue;
            p.drawPixmap(QRect(originX + (tx - txMin) * kTileSize,
                              originY + (ty - tyMin) * kTileSize,
                              kTileSize, kTileSize),
                         loadTile(zoom_, tx, ty));
        }
    }

    // 맵 경계 오버레이
    QColor ov(0, 0, 0, 180);
    int mL = qRound(-viewOffX), mT = qRound(-viewOffY);
    int mR = qRound(totalPx - viewOffX), mB = qRound(totalPx - viewOffY);
    if (mL > 0)  p.fillRect(0,   0,  mL,      H,      ov);
    if (mR < W)  p.fillRect(mR,  0,  W - mR,  H,      ov);
    if (mT > 0)  p.fillRect(0,   0,  W,        mT,     ov);
    if (mB < H)  p.fillRect(0,   mB, W,        H - mB, ov);
}

// ── 네비 모드 렌더링 ─────────────────────────────────────────────────────────

void TileMapWidget::paintRoadEdge(QPainter &p, const RGEdge &edge,
                                  double viewOffX, double viewOffY) {
    if (edge.points.size() < 2) return;

    // 도로 등급 별 두께/색상
    float width;
    QColor color;
    switch (edge.type) {
    case RoadType::Highway: width = 5.0f; color = kRoadHwy;   break;
    case RoadType::Major:   width = 3.5f; color = kRoadMajor; break;
    default:                width = 2.0f; color = kRoadLocal; break;
    }

    // 고속도로는 테두리(outer) + 내부(inner) 이중 선 (현대 네비 스타일)
    if (edge.type == RoadType::Highway) {
        QPainterPath path;
        bool first = true;
        for (const auto &pt : edge.points) {
            double mx, my;
            worldToMap(pt.x(), pt.y(), mx, my);
            QPointF sp(mx - viewOffX, my - viewOffY);
            if (first) { path.moveTo(sp); first = false; }
            else        path.lineTo(sp);
        }
        p.setPen(QPen(kMBlueD, width + 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(path);
        p.setPen(QPen(kRoadHwy, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(path);
        return;
    }

    QPolygonF poly;
    for (const auto &pt : edge.points) {
        double mx, my;
        worldToMap(pt.x(), pt.y(), mx, my);
        poly << QPointF(mx - viewOffX, my - viewOffY);
    }
    p.setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawPolyline(poly);
}

void TileMapWidget::paintNavigation(QPainter &p, int W, int H,
                                    double viewOffX, double viewOffY) {
    // 배경
    p.fillRect(0, 0, W, H, kNavBg);

    if (!road_graph_.isLoaded()) {
        // 그래프 미로드: 안내 문구
        p.setPen(QColor(0x44, 0x44, 0x55));
        p.setFont(QFont("sans", 10));
        p.drawText(rect(), Qt::AlignCenter,
                   "road_graph.json 없음\nCarSim → Bake Road Mask 실행 후\n"
                   "tools/extract_road_graph.py 를 실행하세요");
        return;
    }

    // 뷰포트 월드 범위 계산 (컬링)
    double totalPx = static_cast<double>(kTileSize) * (1 << zoom_);
    double worldW  = world_max_x_ - world_min_x_;
    double worldH  = world_max_z_ - world_min_z_;
    double vMinWx  = world_min_x_ + viewOffX / totalPx * worldW - 50;
    double vMaxWx  = world_min_x_ + (viewOffX + W) / totalPx * worldW + 50;
    double vMinWz  = world_max_z_ - (viewOffY + H) / totalPx * worldH - 50;
    double vMaxWz  = world_max_z_ - viewOffY / totalPx * worldH + 50;

    // 도로 엣지 (Local → Major → Highway 순서로 그려 위에 겹치게)
    p.setRenderHint(QPainter::Antialiasing);
    for (int pass = 0; pass < 3; ++pass) {
        RoadType drawType = (pass == 0) ? RoadType::Local
                          : (pass == 1) ? RoadType::Major
                                        : RoadType::Highway;
        for (const auto &edge : road_graph_.edges()) {
            if (edge.type != drawType) continue;
            if (edge.points.isEmpty()) continue;
            // 간단 컬링: 첫 포인트가 뷰포트 범위 내인지
            double ex = edge.points.first().x(), ez = edge.points.first().y();
            if (ex < vMinWx || ex > vMaxWx || ez < vMinWz || ez > vMaxWz) continue;
            paintRoadEdge(p, edge, viewOffX, viewOffY);
        }
    }

    // A* 경로 강조
    if (!route_.isEmpty()) {
        QPolygonF routePoly;
        for (const auto &pt : route_) {
            double mx, my;
            worldToMap(pt.x(), pt.y(), mx, my);
            routePoly << QPointF(mx - viewOffX, my - viewOffY);
        }
        // 경로 외곽선
        p.setPen(QPen(kMBlueD, 8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolyline(routePoly);
        // 경로 내선
        p.setPen(QPen(kMBlue, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolyline(routePoly);
    }
}

// ── 공통 오버레이 (경로선 위성모드용 + 마커) ─────────────────────────────────

static void drawRouteLine(QPainter &p, const QVector<QPointF> &route,
                          double viewOffX, double viewOffY,
                          auto worldToMap_fn) {
    // 위성 모드에서는 점선으로
    if (route.isEmpty()) return;
    QPolygonF poly;
    for (const auto &pt : route) {
        double mx, my; worldToMap_fn(pt.x(), pt.y(), mx, my);
        poly << QPointF(mx - viewOffX, my - viewOffY);
    }
    p.setPen(QPen(QColor(0x1C, 0x69, 0xD4, 180), 3, Qt::DashLine));
    p.drawPolyline(poly);
}

// ── 메인 페인트 ──────────────────────────────────────────────────────────────

void TileMapWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const int W = width(), H = height();

    double vMx, vMy;
    worldToMap(pos_x_, pos_z_, vMx, vMy);
    double viewOffX = vMx - W / 2.0;
    double viewOffY = vMy - H / 2.0;

    // ── 맵 렌더링 ────────────────────────────────────────────────────────────
    if (map_mode_ == MapMode::Satellite) {
        p.fillRect(rect(), QColor(0x08, 0x08, 0x10));
        paintSatellite(p, W, H, viewOffX, viewOffY);

        // 위성 모드에서 경로: 점선 오버레이
        if (has_dest_ && !route_.isEmpty()) {
            // 도로 경로 있으면 폴리라인, 없으면 직선
            const QVector<QPointF> &pts = route_.isEmpty()
                ? QVector<QPointF>{{dest_x_, dest_z_}} : route_;
            QPolygonF poly;
            poly << QPointF(W / 2.0, H / 2.0);
            for (const auto &pt : pts) {
                double mx, my; worldToMap(pt.x(), pt.y(), mx, my);
                poly << QPointF(mx - viewOffX, my - viewOffY);
            }
            p.setPen(QPen(kMBlue.darker(130), 4, Qt::DashLine, Qt::RoundCap));
            p.drawPolyline(poly);
            p.setPen(QPen(kMBlue, 2.5, Qt::DashLine, Qt::RoundCap));
            p.drawPolyline(poly);
        } else if (has_dest_ && route_.isEmpty()) {
            // 직선 대체
            double destMx, destMy;
            worldToMap(dest_x_, dest_z_, destMx, destMy);
            QPen rp(kMBlue, 2.5, Qt::DashLine); rp.setDashPattern({6, 5});
            p.setPen(rp);
            p.drawLine(QPointF(W/2.0, H/2.0),
                       QPointF(destMx - viewOffX, destMy - viewOffY));
        }
    } else {
        paintNavigation(p, W, H, viewOffX, viewOffY);
    }

    // ── 목적지 핀 ────────────────────────────────────────────────────────────
    if (has_dest_) {
        double destMx, destMy;
        worldToMap(dest_x_, dest_z_, destMx, destMy);
        double dsx = destMx - viewOffX, dsy = destMy - viewOffY;

        p.save();
        p.translate(dsx, dsy);
        // 핀 막대
        QPainterPath pin;
        pin.moveTo(0, 0);
        pin.lineTo(0, -22);
        p.setPen(QPen(kMBlueD, 2.5));
        p.setBrush(Qt::NoBrush);
        p.drawPath(pin);
        // 핀 원
        p.setBrush(kMBlue);
        p.setPen(QPen(Qt::white, 1.5));
        p.drawEllipse(QPoint(0, -28), 7, 7);
        p.restore();
    }

    // ── 차량 마커 (화면 정중앙) ──────────────────────────────────────────────
    {
        p.save();
        p.translate(W / 2, H / 2);
        p.rotate(heading_);

        // 방향 그림자
        p.setBrush(kMBlue.darker(180));
        p.setPen(Qt::NoPen);
        p.translate(1, 2);
        QPainterPath arrow;
        arrow.moveTo(0, -13); arrow.lineTo(-7, 6);
        arrow.lineTo(0, 2);   arrow.lineTo(7, 6);
        arrow.closeSubpath();
        p.drawPath(arrow);
        p.translate(-1, -2);

        // 외곽 글로우
        p.setBrush(QColor(0x1C, 0x69, 0xD4, 50));
        p.drawEllipse(QPoint(0, 0), 14, 14);

        // 화살표
        p.setBrush(kMBlue);
        p.setPen(QPen(Qt::white, 1.2));
        p.drawPath(arrow);
        p.restore();
    }

    // ── 우하단: 줌 + 모드 표시 ───────────────────────────────────────────────
    {
        QFont f; f.setPointSize(8);
        p.setFont(f);
        p.setPen(QColor(0x44, 0x44, 0x66));
        QString info = (map_mode_ == MapMode::Navigation ? "NAV" : "SAT");
        info += auto_zoom_ ? QStringLiteral("  z%1·AUTO").arg(zoom_)
                           : QStringLiteral("  z%1").arg(zoom_);
        p.drawText(QRect(W - 100, H - 18, 96, 14),
                   Qt::AlignRight | Qt::AlignVCenter, info);
    }
}
