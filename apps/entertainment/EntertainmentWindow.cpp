#include "EntertainmentWindow.hpp"
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

    // NAV 아이콘 (active)
    QRect navRect((width() - 40) / 2, 52, 40, 40);
    p.fillRect(navRect, QColor(0x1A, 0x1A, 0x1A));
    p.setPen(QPen(kFg1, 1));
    p.drawRect(navRect);
    p.setPen(QPen(kFg1, 1.5f, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
    p.setBrush(Qt::NoBrush);
    drawNavIcon(p, navRect);

    // 설정 아이콘 (하단)
    QRect settingsRect((width() - 40) / 2, height() - 50, 40, 40);
    p.setPen(QPen(kFg3, 1.5f, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
    drawSettingsIcon(p, settingsRect);
}

static QPointF ip(QRect r, float x, float y) {
    return QPointF(r.x() + r.width() * 0.5f + (x - 11.0f),
                   r.y() + r.height() * 0.5f + (y - 11.0f));
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
// NavScreen
// ═══════════════════════════════════════════════════════════════════════════════

NavScreen::NavScreen(QWidget *parent) : QWidget(parent) {
    tile_map_ = new TileMapWidget(this);
    buildETACard();
    buildSpeedBadge();
    buildSpeedLimit();
    buildZoomCtrl();
}

void NavScreen::buildETACard() {
    eta_card_ = new QWidget(this);

    auto *vb = new QVBoxLayout(eta_card_);
    vb->setContentsMargins(18, 12, 18, 12);
    vb->setSpacing(0);

    auto *dest = new QLabel("NAVIGATION · DEMO CITY", eta_card_);
    {
        QFont f; f.setPointSize(7); f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1);
        dest->setFont(f);
        dest->setStyleSheet("color: #7e7e7e;");
    }
    vb->addWidget(dest);
    vb->addSpacing(6);

    auto *row = new QHBoxLayout;
    row->setSpacing(8);

    auto *zoomLbl = new QLabel("ZOOM", eta_card_);
    {
        QFont f; f.setPointSize(7); f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1);
        zoomLbl->setFont(f);
        zoomLbl->setStyleSheet("color: #7e7e7e;");
    }
    row->addWidget(zoomLbl);

    auto *posLbl = new QLabel("GPS · ACTIVE", eta_card_);
    {
        QFont f; f.setPointSize(7);
        posLbl->setFont(f);
        posLbl->setStyleSheet(QString("color: %1;").arg(kOK.name()));
    }
    row->addWidget(posLbl);
    row->addStretch();
    vb->addLayout(row);

    // M stripe 하단
    auto *stripe = new QWidget(eta_card_);
    stripe->setFixedHeight(2);
    stripe->setStyleSheet(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #0066b1,stop:0.333 #0066b1,"
        "stop:0.334 #1c69d4,stop:0.666 #1c69d4,"
        "stop:0.667 #e22718,stop:1 #e22718);");
    vb->addSpacing(8);
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
}

void NavScreen::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    layoutOverlays();
}

void NavScreen::layoutOverlays() {
    const int W = width(), H = height();
    const int pad = 12;

    tile_map_->setGeometry(0, 0, W, H);

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

    nav_screen_ = new NavScreen(right);
    vb->addWidget(nav_screen_, 1);

    // 시계
    connect(&clock_timer_, &QTimer::timeout,
            status_bar_, &StatusBarWidget::updateClock);
    clock_timer_.start(1000);
}

void EntertainmentWindow::setModel(EntertainmentModel *model) {
    model_ = model;
    connect(model_, &EntertainmentModel::positionChanged,
            nav_screen_->tileMap(), &TileMapWidget::setPosition);
    connect(model_, &EntertainmentModel::headingChanged,
            nav_screen_->tileMap(), &TileMapWidget::setHeading);
    connect(model_, &EntertainmentModel::speedChanged,
            nav_screen_, &NavScreen::setSpeed);
}
