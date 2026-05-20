#include "EntertainmentWindow.hpp"
#include "SwitchPanelWidget.hpp"
#include "VehicleInfoWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QDateTime>

static const QColor kBg    {0x00, 0x00, 0x00};
static const QColor kHair  {0x26, 0x26, 0x26};
static const QColor kHair2 {0x3C, 0x3C, 0x3C};
static const QColor kMBlueL{0x00, 0x66, 0xB1};
static const QColor kMBlue {0x1C, 0x69, 0xD4};
static const QColor kMRed  {0xE2, 0x27, 0x18};
static const QColor kWarn  {0xF4, 0xB4, 0x00};
static const QColor kOK    {0x0F, 0xA3, 0x36};
static const QColor kFg1   {0xFF, 0xFF, 0xFF};
static const QColor kFg3   {0x7E, 0x7E, 0x7E};

// ═══════════════════════════════════════════════════════════════════════════════
// SideRailWidget
// ═══════════════════════════════════════════════════════════════════════════════

SideRailWidget::SideRailWidget(QWidget *parent) : QWidget(parent) {
    setFixedWidth(56);
}

void SideRailWidget::setCurrentPage(int page) {
    currentPage_ = page;
    update();
}

void SideRailWidget::mouseReleaseEvent(QMouseEvent *e) {
    QRect navRect   ((width() - 40) / 2, 52,  40, 40);
    QRect switchRect((width() - 40) / 2, 100, 40, 40);
    QRect infoRect  ((width() - 40) / 2, 148, 40, 40);

    if (navRect.contains(e->pos())) {
        currentPage_ = 0;
        emit pageRequested(0);
        update();
    } else if (switchRect.contains(e->pos())) {
        currentPage_ = 1;
        emit pageRequested(1);
        update();
    } else if (infoRect.contains(e->pos())) {
        currentPage_ = 2;
        emit pageRequested(2);
        update();
    }
    QWidget::mouseReleaseEvent(e);
}

void SideRailWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), kBg);

    // 우측 구분선
    p.setPen(QPen(kHair, 1));
    p.drawLine(width() - 1, 0, width() - 1, height());

    // M 트라이컬러 세로 스트라이프 (3×28 px)
    int sx = (width() - 3) / 2;
    p.fillRect(sx, 10,      3, 9,  kMBlueL);
    p.fillRect(sx, 10 + 9,  3, 10, kMBlue);
    p.fillRect(sx, 10 + 19, 3, 9,  kMRed);

    // ── NAV 아이콘 ────────────────────────────────────────────────────────────
    {
        QRect navRect((width() - 40) / 2, 52, 40, 40);
        bool  active = (currentPage_ == 0);
        p.fillRect(navRect, active ? QColor(0x1A, 0x1A, 0x1A) : QColor(0x0A, 0x0A, 0x0A));
        // 활성 페이지 좌측 인디케이터 바
        if (active) p.fillRect(0, navRect.top(), 3, navRect.height(), kMBlue);
        p.setPen(QPen(active ? kFg1 : kFg3,
                      active ? 1.5f : 1.0f,
                      Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        p.setBrush(Qt::NoBrush);
        drawNavIcon(p, navRect);
    }

    // ── SWITCH 아이콘 ─────────────────────────────────────────────────────────
    {
        QRect switchRect((width() - 40) / 2, 100, 40, 40);
        bool  active = (currentPage_ == 1);
        p.fillRect(switchRect, active ? QColor(0x1A, 0x1A, 0x1A) : QColor(0x0A, 0x0A, 0x0A));
        if (active) p.fillRect(0, switchRect.top(), 3, switchRect.height(), kMBlue);
        p.setPen(QPen(active ? kFg1 : kFg3, 1));
        p.setBrush(Qt::NoBrush);
        drawSwitchIcon(p, switchRect);
    }

    // ── VEHICLE INFO 아이콘 ───────────────────────────────────────────────────
    {
        QRect infoRect((width() - 40) / 2, 148, 40, 40);
        bool  active = (currentPage_ == 2);
        p.fillRect(infoRect, active ? QColor(0x1A, 0x1A, 0x1A) : QColor(0x0A, 0x0A, 0x0A));
        if (active) p.fillRect(0, infoRect.top(), 3, infoRect.height(), kMBlue);
        p.setPen(QPen(active ? kFg1 : kFg3, 1.5f, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        p.setBrush(Qt::NoBrush);
        drawInfoIcon(p, infoRect);
    }

    // ── 설정 아이콘 (하단) ────────────────────────────────────────────────────
    QRect settingsRect((width() - 40) / 2, height() - 50, 40, 40);
    p.setPen(QPen(kFg3, 1.5f, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
    drawSettingsIcon(p, settingsRect);
}

static QPointF ip(QRect r, float x, float y) {
    return QPointF(r.x() + r.width() * 0.5f + (x - 11.0f),
                   r.y() + r.height() * 0.5f + (y - 11.0f));
}

// 스위치 패널 아이콘: 2×2 그리드 + 토글 바
void SideRailWidget::drawSwitchIcon(QPainter &p, QRect r) const {
    QColor col = p.pen().color();
    int cx = r.x() + r.width()  / 2;
    int cy = r.y() + r.height() / 2;
    const int sq  = 5;
    const int gap = 4;
    // 4개의 작은 사각형 (스위치들)
    p.fillRect(cx - sq - gap, cy - sq - gap, sq, sq, col);
    p.fillRect(cx + gap,      cy - sq - gap, sq, sq, col);
    p.fillRect(cx - sq - gap, cy + gap,      sq, sq, col);
    p.fillRect(cx + gap,      cy + gap,      sq, sq, col);
    // 하단 수평선 (토글 심볼)
    p.setPen(QPen(col, 1.5f));
    p.drawLine(cx - sq - gap, cy + gap + sq + 4, cx + gap + sq, cy + gap + sq + 4);
}

// 차량 정보 아이콘: 속도계 (반원 + 바늘 모양)
void SideRailWidget::drawInfoIcon(QPainter &p, QRect r) const {
    QColor col = p.pen().color();
    int cx = r.x() + r.width()  / 2;
    int cy = r.y() + r.height() / 2 + 2;
    const int R = 10;
    // 반원 (속도계 모양)
    p.drawArc(cx - R, cy - R, R * 2, R * 2, 0, 180 * 16);
    // 바늘 (45도 방향)
    p.drawLine(cx, cy, cx + 6, cy - 7);
    // 중심점
    p.setBrush(col);
    p.drawEllipse(QPoint(cx, cy), 2, 2);
    p.setBrush(Qt::NoBrush);
    // 눈금 3개
    p.drawLine(cx - R, cy, cx - R + 3, cy);
    p.drawLine(cx, cy - R, cx, cy - R + 3);
    p.drawLine(cx + R, cy, cx + R - 3, cy);
}

void SideRailWidget::drawNavIcon(QPainter &p, QRect r) const {
    QPainterPath path;
    path.moveTo(ip(r, 3,  18));
    path.lineTo(ip(r, 11, 3));
    path.lineTo(ip(r, 19, 18));
    path.lineTo(ip(r, 11, 14));
    path.closeSubpath();
    p.drawPath(path);
}

void SideRailWidget::drawSettingsIcon(QPainter &p, QRect r) const {
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(ip(r, 10, 10), 3.0, 3.0);
    p.drawLine(ip(r, 10, 2),  ip(r, 10, 4));
    p.drawLine(ip(r, 10, 18), ip(r, 10, 16));
    p.drawLine(ip(r, 2,  10), ip(r, 4,  10));
    p.drawLine(ip(r, 18, 10), ip(r, 16, 10));
    p.drawLine(ip(r, 4.5, 4.5),   ip(r, 5.9, 5.9));
    p.drawLine(ip(r, 15.5, 15.5), ip(r, 14.1, 14.1));
    p.drawLine(ip(r, 4.5, 15.5),  ip(r, 5.9, 14.1));
    p.drawLine(ip(r, 15.5, 4.5),  ip(r, 14.1, 5.9));
}

// ═══════════════════════════════════════════════════════════════════════════════
// StatusBarWidget
// ═══════════════════════════════════════════════════════════════════════════════

StatusBarWidget::StatusBarWidget(QWidget *parent) : QWidget(parent) {
    setFixedHeight(28);
    updateClock();
}

void StatusBarWidget::updateClock() {
    time_str_ = QDateTime::currentDateTime().toString("HH:mm");
    date_str_ = QDateTime::currentDateTime().toString("ddd · d MMM").toUpper();
    update();
}

void StatusBarWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 153));
    p.setPen(QPen(kHair, 1));
    p.drawLine(0, height() - 1, width(), height() - 1);

    const int pad = 16;
    const int cy  = height() / 2;

    // 시각
    QFont f; f.setPointSize(9); f.setBold(true);
    p.setFont(f); p.setPen(kFg1);
    QFontMetrics fm(f);
    p.drawText(pad, cy + fm.ascent() / 2, time_str_);

    // 날짜
    QFont fd; fd.setPointSize(7);
    p.setFont(fd); p.setPen(kFg3);
    QFontMetrics fmd(fd);
    p.drawText(pad + fm.horizontalAdvance(time_str_) + 12,
               cy + fmd.ascent() / 2, date_str_);

    // 우측: BT + CAN
    {
        QFont fr; fr.setPointSize(7);
        p.setFont(fr);
        QFontMetrics fmr(fr);

        int rx = width() - pad;

        // CAN 상태
        QString can = "● CAN";
        p.setPen(kOK);
        p.drawText(rx - fmr.horizontalAdvance(can), cy + fmr.ascent() / 2, can);
        rx -= fmr.horizontalAdvance(can) + 14;

        // BT
        QString bt = "● BT";
        p.setPen(kOK);
        p.drawText(rx - fmr.horizontalAdvance(bt), cy + fmr.ascent() / 2, bt);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// ManeuverWidget
// ═══════════════════════════════════════════════════════════════════════════════

ManeuverWidget::ManeuverWidget(QWidget *parent) : QWidget(parent) {
    setFixedSize(90, 90);
    hide();   // 경로 없을 때 숨김
}

void ManeuverWidget::updateManeuver(TileMapWidget::Maneuver type, double distMeters) {
    type_  = type;
    dist_  = distMeters;
    bool visible = (type != TileMapWidget::Maneuver::None);
    setVisible(visible);
    if (visible) update();
}

void ManeuverWidget::paintEvent(QPaintEvent *) {
    using M = TileMapWidget::Maneuver;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 배경
    p.setBrush(QColor(0, 0, 0, 210));
    p.setPen(QPen(QColor(0x3c, 0x3c, 0x3c), 1));
    p.drawRoundedRect(rect().adjusted(1,1,-1,-1), 8, 8);

    const int W = width(), H = height();
    const int arrowAreaH = 58;

    // ── 화살표 ──────────────────────────────────────────────────────────────
    p.save();
    p.translate(W / 2.0, arrowAreaH / 2.0 + 4);
    p.setPen(Qt::NoPen);

    if (type_ == M::Arrived) {
        // 원 + 체크
        p.setBrush(QColor(0x0f, 0xa3, 0x36));
        p.drawEllipse(QPoint(0, 0), 20, 20);
        p.setPen(QPen(Qt::white, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        QPainterPath chk;
        chk.moveTo(-9, 0); chk.lineTo(-3, 7); chk.lineTo(9, -7);
        p.drawPath(chk);
    } else {
        // 직진 or 좌/우 화살표
        QPainterPath arrow;
        if (type_ == M::Straight) {
            // ↑ 위쪽 화살표
            arrow.moveTo( 0, -22);
            arrow.lineTo(-9,  -9);
            arrow.lineTo(-4,  -9);
            arrow.lineTo(-4,  18);
            arrow.lineTo( 4,  18);
            arrow.lineTo( 4,  -9);
            arrow.lineTo( 9,  -9);
            arrow.closeSubpath();
        } else if (type_ == M::TurnRight) {
            // → 우회전
            arrow.moveTo( 22,  0);
            arrow.lineTo(  9, -9);
            arrow.lineTo(  9, -4);
            arrow.lineTo(-14, -4);
            arrow.lineTo(-14, -14);
            arrow.lineTo(-22,  0);
            arrow.lineTo(-14, 14);
            arrow.lineTo(-14,  4);
            arrow.lineTo(  9,  4);
            arrow.lineTo(  9,  9);
            arrow.closeSubpath();
        } else {
            // ← 좌회전 (TurnRight 좌우 반전)
            p.scale(-1, 1);
            arrow.moveTo( 22,  0);
            arrow.lineTo(  9, -9);
            arrow.lineTo(  9, -4);
            arrow.lineTo(-14, -4);
            arrow.lineTo(-14, -14);
            arrow.lineTo(-22,  0);
            arrow.lineTo(-14, 14);
            arrow.lineTo(-14,  4);
            arrow.lineTo(  9,  4);
            arrow.lineTo(  9,  9);
            arrow.closeSubpath();
        }
        p.setBrush(QColor(0x1c, 0x69, 0xd4));
        p.drawPath(arrow);
    }
    p.restore();

    // ── 거리 텍스트 ──────────────────────────────────────────────────────────
    if (dist_ > 0 && type_ != M::Arrived) {
        QString txt = dist_ >= 1000.0
            ? QStringLiteral("%1 km").arg(dist_ / 1000.0, 0, 'f', 1)
            : QStringLiteral("%1 m").arg(static_cast<int>(dist_));
        QFont f; f.setPointSize(8); f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(0xff, 0xff, 0xff));
        p.drawText(QRect(0, arrowAreaH, W, H - arrowAreaH),
                   Qt::AlignHCenter | Qt::AlignVCenter, txt);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// NavScreen
// ═══════════════════════════════════════════════════════════════════════════════

NavScreen::NavScreen(QWidget *parent) : QWidget(parent) {
    tile_map_ = new TileMapWidget(this);
    maneuver_widget_ = new ManeuverWidget(this);
    buildETACard();
    buildSpeedBadge();
    buildSpeedLimit();
    buildZoomCtrl();

    connect(tile_map_, &TileMapWidget::maneuverChanged,
            maneuver_widget_, &ManeuverWidget::updateManeuver);
}

void NavScreen::buildETACard() {
    eta_card_ = new QWidget(this);

    auto *vb = new QVBoxLayout(eta_card_);
    vb->setContentsMargins(18, 12, 18, 12);
    vb->setSpacing(0);

    // 타이틀 (경로 없을 때: "NAVIGATION · DEMO CITY", 있을 때: "▶ ROUTE SET")
    eta_title_label_ = new QLabel("NAVIGATION · DEMO CITY", eta_card_);
    {
        QFont f; f.setPointSize(7); f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1);
        eta_title_label_->setFont(f);
        eta_title_label_->setStyleSheet("color: #7e7e7e;");
    }
    vb->addWidget(eta_title_label_);
    vb->addSpacing(4);

    // 거리 레이블 (경로 없을 때: "GPS · ACTIVE", 있을 때: "1234 m")
    eta_dist_label_ = new QLabel("GPS · ACTIVE", eta_card_);
    {
        QFont f; f.setPointSize(11); f.setBold(true);
        eta_dist_label_->setFont(f);
        eta_dist_label_->setStyleSheet(QString("color: %1;").arg(kOK.name()));
    }
    vb->addWidget(eta_dist_label_);
    vb->addSpacing(2);

    // 지도 클릭 안내 (작은 힌트 텍스트)
    auto *hint = new QLabel("지도 클릭으로 경로 설정", eta_card_);
    {
        QFont f; f.setPointSize(6);
        hint->setFont(f);
        hint->setStyleSheet("color: #444444;");
    }
    vb->addWidget(hint);

    // M stripe 하단
    auto *stripe = new QWidget(eta_card_);
    stripe->setFixedHeight(2);
    stripe->setStyleSheet(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #0066b1,stop:0.333 #0066b1,"
        "stop:0.334 #1c69d4,stop:0.666 #1c69d4,"
        "stop:0.667 #e22718,stop:1 #e22718);");
    vb->addSpacing(6);
    vb->addWidget(stripe);
}

void NavScreen::buildSpeedBadge() {
    speed_badge_ = new QWidget(this);
    auto *hb = new QHBoxLayout(speed_badge_);
    hb->setContentsMargins(10, 6, 10, 6);
    hb->setSpacing(4);

    speed_val_label_ = new QLabel("0", speed_badge_);
    {
        QFont f; f.setPointSize(14); f.setBold(true);
        speed_val_label_->setFont(f);
        speed_val_label_->setStyleSheet("color: #ffffff;");
    }
    auto *unit = new QLabel("KM/H", speed_badge_);
    {
        QFont f; f.setPointSize(7); f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1);
        unit->setFont(f);
        unit->setStyleSheet("color: #7e7e7e;");
        unit->setAlignment(Qt::AlignBottom);
    }
    hb->addWidget(speed_val_label_);
    hb->addWidget(unit);
}

void NavScreen::buildSpeedLimit() {
    speed_limit_ = new QWidget(this);
    speed_limit_->setFixedSize(56, 56);
    speed_limit_->setStyleSheet(
        "background: #ffffff;"
        "border-radius: 28px;"
        "border: 4px solid #e22718;");
    auto *lbl = new QLabel("80", speed_limit_);
    lbl->setGeometry(0, 0, 56, 56);
    lbl->setAlignment(Qt::AlignCenter);
    QFont f; f.setPointSize(14); f.setBold(true);
    lbl->setFont(f);
    lbl->setStyleSheet("color: #000000; border: none; background: transparent;");
    speed_limit_->hide();   // CAN 데이터 없음 — 실제 제한속도 수신 시 show() 연결
}

void NavScreen::buildZoomCtrl() {
    zoom_ctrl_ = new QWidget(this);
    zoom_ctrl_->setFixedSize(32, 65);
    zoom_ctrl_->setStyleSheet("background: rgba(0,0,0,178); border: 1px solid #3c3c3c;");

    auto *vb = new QVBoxLayout(zoom_ctrl_);
    vb->setContentsMargins(0, 0, 0, 0);
    vb->setSpacing(0);

    auto makeBtn = [&](const QString &text) {
        auto *btn = new QPushButton(text, zoom_ctrl_);
        btn->setFixedSize(32, 32);
        btn->setStyleSheet(
            "QPushButton { background: transparent; border: none; color: #ffffff; font-size: 16px; }"
            "QPushButton:pressed { background: #1a1a1a; }");
        return btn;
    };

    auto *zIn  = makeBtn("+");
    auto *div  = new QWidget(zoom_ctrl_);
    div->setFixedHeight(1);
    div->setStyleSheet("background: #3c3c3c;");
    auto *zOut = makeBtn("−");

    connect(zIn,  &QPushButton::clicked, tile_map_, [this]{ tile_map_->setZoom(tile_map_->zoom() + 1); });
    connect(zOut, &QPushButton::clicked, tile_map_, [this]{ tile_map_->setZoom(tile_map_->zoom() - 1); });

    vb->addWidget(zIn);
    vb->addWidget(div);
    vb->addWidget(zOut);
}

void NavScreen::setSpeed(int kmh) {
    if (!speed_val_label_) return;
    speed_val_label_->setText(QString::number(kmh));
    QString col;
    if      (kmh < 80)  col = "#ffffff";
    else if (kmh < 130) col = "#f4b400";
    else                col = "#e22718";
    speed_val_label_->setStyleSheet(QStringLiteral("color: %1;").arg(col));

    // auto-zoom 전달
    if (tile_map_) tile_map_->setSpeedKmh(static_cast<double>(kmh));
}

void NavScreen::onDistanceChanged(double meters) {
    if (!eta_title_label_ || !eta_dist_label_) return;
    if (meters < 0) {
        // 경로 없음
        eta_title_label_->setText("NAVIGATION · DEMO CITY");
        eta_title_label_->setStyleSheet("color: #7e7e7e;");
        eta_dist_label_->setText("GPS · ACTIVE");
        eta_dist_label_->setStyleSheet(QString("color: %1;").arg(kOK.name()));
        eta_dist_label_->setFont(QFont(eta_dist_label_->font().family(), 7));
    } else {
        // 경로 있음: 거리 표시
        eta_title_label_->setText("ROUTE ACTIVE");
        eta_title_label_->setStyleSheet("color: #1c69d4;");
        QString distStr = meters >= 1000.0
            ? QStringLiteral("%1 km").arg(meters / 1000.0, 0, 'f', 1)
            : QStringLiteral("%1 m").arg(static_cast<int>(meters));
        eta_dist_label_->setText(distStr);
        QFont f = eta_dist_label_->font(); f.setPointSize(11);
        eta_dist_label_->setFont(f);
        eta_dist_label_->setStyleSheet("color: #ffffff;");
    }
}

void NavScreen::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    layoutOverlays();
}

void NavScreen::layoutOverlays() {
    const int W = width(), H = height();
    const int pad = 12;

    tile_map_->setGeometry(0, 0, W, H);

    // 회전 안내 (상단 중앙)
    if (maneuver_widget_) {
        int mw = maneuver_widget_->width();
        maneuver_widget_->move((W - mw) / 2, pad);
    }

    // 속도 배지 (좌상단)
    if (speed_badge_) {
        speed_badge_->adjustSize();
        speed_badge_->setGeometry(pad, pad, speed_badge_->width(), 34);
        speed_badge_->setStyleSheet(
            "background: rgba(0,0,0,217); border: 1px solid #262626;");
    }

    // ETA/상태 카드 (좌하단)
    if (eta_card_) {
        eta_card_->setGeometry(pad, H - 90 - pad, 220, 90);
        eta_card_->setStyleSheet(
            "background: rgba(0,0,0,217); border: 1px solid #262626;");
    }

    // 속도제한 (우하단, 줌 컨트롤 위)
    if (speed_limit_) {
        speed_limit_->move(W - 56 - pad, H - 56 - 12 - 65 - pad);
    }

    // 줌 컨트롤 (우하단)
    if (zoom_ctrl_) {
        zoom_ctrl_->setGeometry(W - 32 - pad, H - 65 - pad, 32, 65);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// EntertainmentWindow
// ═══════════════════════════════════════════════════════════════════════════════

EntertainmentWindow::EntertainmentWindow(QWidget *parent) : QWidget(parent) {
    setMinimumSize(800, 480);
    setStyleSheet("background: #000000;");

    // 사이드 레일 (56px, 좌측 고정)
    rail_ = new SideRailWidget(this);
    rail_->setGeometry(0, 0, 56, 480);

    // 우측 영역 (744px)
    auto *right = new QWidget(this);
    right->setGeometry(56, 0, 744, 480);

    auto *vb = new QVBoxLayout(right);
    vb->setContentsMargins(0, 0, 0, 0);
    vb->setSpacing(0);

    status_bar_ = new StatusBarWidget(right);
    vb->addWidget(status_bar_);

    // 페이지 스택: 0 = NavScreen, 1 = SwitchPanelWidget, 2 = VehicleInfoWidget
    pages_ = new QStackedWidget(right);

    nav_screen_   = new NavScreen(pages_);
    switch_panel_ = new SwitchPanelWidget(pages_);
    vehicle_info_ = new VehicleInfoWidget(pages_);
    pages_->addWidget(nav_screen_);
    pages_->addWidget(switch_panel_);
    pages_->addWidget(vehicle_info_);
    pages_->setCurrentIndex(0);

    vb->addWidget(pages_, 1);

    // 지도 경계 (MapTileBaker 기본값과 일치)
    nav_screen_->tileMap()->setWorldBounds(-400.0, 450.0, -200.0, 240.0);
    nav_screen_->tileMap()->setZoom(2);

    // SideRail ↔ 페이지 전환
    connect(rail_, &SideRailWidget::pageRequested,
            pages_, &QStackedWidget::setCurrentIndex);

    // 시계
    connect(&clock_timer_, &QTimer::timeout,
            status_bar_, &StatusBarWidget::updateClock);
    clock_timer_.start(1000);
}

bool EntertainmentWindow::loadRoadGraph(const QString &jsonPath) {
    return nav_screen_->tileMap()->loadRoadGraph(jsonPath);
}

void EntertainmentWindow::setModel(EntertainmentModel *model) {
    model_ = model;
    connect(model_, &EntertainmentModel::positionChanged,
            nav_screen_->tileMap(), &TileMapWidget::setPosition);
    connect(model_, &EntertainmentModel::headingChanged,
            nav_screen_->tileMap(), &TileMapWidget::setHeading);
    connect(model_, &EntertainmentModel::speedChanged,
            nav_screen_, &NavScreen::setSpeed);

    // TileMapWidget 경로 거리 → NavScreen 카드 업데이트
    connect(nav_screen_->tileMap(), &TileMapWidget::distanceToDestChanged,
            nav_screen_, &NavScreen::onDistanceChanged);

    // 스위치 패널 ↔ 모델 연결
    switch_panel_->setModel(model_);

    // 차량 정보 ↔ 모델 연결
    vehicle_info_->setModel(model_);

    // 내비 방향 → 클러스터 브로드캐스트 (추후 0x700)
    // TODO: maneuver CAN broadcast
}
