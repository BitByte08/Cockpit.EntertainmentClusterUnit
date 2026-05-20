#include "VehicleInfoWidget.hpp"
#include "models/EntertainmentModel.hpp"
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFont>

// ── 색상 팔레트 (기존 EntertainmentWindow 동일) ────────────────────────────────
static const QColor kBg    {0x00, 0x00, 0x00};
static const QColor kHair  {0x26, 0x26, 0x26};
static const QColor kMBlueL{0x00, 0x66, 0xB1};
static const QColor kMBlue {0x1C, 0x69, 0xD4};
static const QColor kMRed  {0xE2, 0x27, 0x18};
static const QColor kOK    {0x0F, 0xA3, 0x36};
static const QColor kWarn  {0xF4, 0xB4, 0x00};
static const QColor kFg1   {0xFF, 0xFF, 0xFF};
static const QColor kFg3   {0x7E, 0x7E, 0x7E};

// ── 카드 위젯 ─────────────────────────────────────────────────────────────────
static QWidget *makeCard(QWidget *parent) {
    auto *w = new QWidget(parent);
    w->setStyleSheet("background: #0a0a0a; border: 1px solid #262626; border-radius: 6px;");
    return w;
}

static QLabel *makeCaption(const QString &text, QWidget *parent) {
    auto *lbl = new QLabel(text, parent);
    QFont f; f.setPointSize(7); f.setBold(true);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
    lbl->setFont(f);
    lbl->setStyleSheet("color: #7e7e7e; background: transparent; border: none;");
    return lbl;
}

static QLabel *makeValueLabel(const QString &text, int ptSize, QWidget *parent) {
    auto *lbl = new QLabel(text, parent);
    QFont f; f.setPointSize(ptSize); f.setBold(true);
    lbl->setFont(f);
    lbl->setStyleSheet("color: #ffffff; background: transparent; border: none;");
    lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    return lbl;
}

static QLabel *makeUnit(const QString &text, QWidget *parent) {
    auto *lbl = new QLabel(text, parent);
    QFont f; f.setPointSize(7); f.setBold(true);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    lbl->setFont(f);
    lbl->setStyleSheet("color: #7e7e7e; background: transparent; border: none;");
    lbl->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
    return lbl;
}

// ═══════════════════════════════════════════════════════════════════════════════
// VehicleInfoWidget
// ═══════════════════════════════════════════════════════════════════════════════

VehicleInfoWidget::VehicleInfoWidget(QWidget *parent) : QWidget(parent) {
    buildUI();
}

void VehicleInfoWidget::setModel(EntertainmentModel *model) {
    model_ = model;
    connect(model_, &EntertainmentModel::speedChanged,
            this, &VehicleInfoWidget::onSpeedChanged);
    connect(model_, &EntertainmentModel::rpmChanged,
            this, &VehicleInfoWidget::onRpmChanged);
    connect(model_, &EntertainmentModel::gearChanged,
            this, &VehicleInfoWidget::onGearChanged);
    connect(model_, &EntertainmentModel::engineStateChanged,
            this, &VehicleInfoWidget::onEngineStateChanged);

    // 초기값 동기화
    onSpeedChanged(model_->speed());
    onRpmChanged(model_->rpm());
    onGearChanged(model_->gear());
    onEngineStateChanged(model_->coolant(), model_->oilPct(), model_->fuelPct());
}

void VehicleInfoWidget::buildUI() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 12);
    root->setSpacing(8);

    // ── 헤더 ─────────────────────────────────────────────────────────────────
    {
        auto *header = new QWidget(this);
        header->setFixedHeight(34);
        auto *hb = new QHBoxLayout(header);
        hb->setContentsMargins(0, 0, 0, 0);

        auto *title = new QLabel("VEHICLE INFO", header);
        QFont f; f.setPointSize(10); f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
        title->setFont(f);
        title->setStyleSheet("color: #ffffff;");
        hb->addWidget(title);
        hb->addStretch();

        auto *stripe = new QWidget(header);
        stripe->setFixedSize(20, 24);
        stripe->setStyleSheet(
            "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "stop:0 #0066b1, stop:0.333 #0066b1,"
            "stop:0.334 #1c69d4, stop:0.666 #1c69d4,"
            "stop:0.667 #e22718, stop:1 #e22718);");
        hb->addWidget(stripe);
        root->addWidget(header);
    }
    {
        auto *sep = new QWidget(this);
        sep->setFixedHeight(1);
        sep->setStyleSheet("background: #262626;");
        root->addWidget(sep);
    }

    // ── 상단: 속도 · RPM · 기어 ───────────────────────────────────────────────
    {
        auto *row = new QWidget(this);
        auto *hb  = new QHBoxLayout(row);
        hb->setContentsMargins(0, 0, 0, 0);
        hb->setSpacing(8);

        // 속도 카드
        {
            auto *card = makeCard(row);
            auto *vb   = new QVBoxLayout(card);
            vb->setContentsMargins(14, 10, 14, 10);
            vb->setSpacing(2);
            vb->addWidget(makeCaption("SPEED", card));
            auto *numRow = new QHBoxLayout();
            numRow->setSpacing(4);
            lbl_speed_val_ = makeValueLabel("0", 26, card);
            numRow->addWidget(lbl_speed_val_);
            numRow->addWidget(makeUnit("KM/H", card));
            numRow->addStretch();
            vb->addLayout(numRow);
            hb->addWidget(card, 3);
        }

        // RPM 카드
        {
            auto *card = makeCard(row);
            auto *vb   = new QVBoxLayout(card);
            vb->setContentsMargins(14, 10, 14, 10);
            vb->setSpacing(2);
            vb->addWidget(makeCaption("RPM", card));
            auto *numRow = new QHBoxLayout();
            numRow->setSpacing(4);
            lbl_rpm_val_ = makeValueLabel("0", 20, card);
            numRow->addWidget(lbl_rpm_val_);
            numRow->addWidget(makeUnit("RPM", card));
            numRow->addStretch();
            vb->addLayout(numRow);
            hb->addWidget(card, 3);
        }

        // 기어 카드
        {
            auto *card = makeCard(row);
            auto *vb   = new QVBoxLayout(card);
            vb->setContentsMargins(14, 10, 14, 10);
            vb->setSpacing(2);
            vb->addWidget(makeCaption("GEAR", card));
            lbl_gear_val_ = makeValueLabel("N", 26, card);
            lbl_gear_val_->setAlignment(Qt::AlignCenter);
            vb->addWidget(lbl_gear_val_);
            hb->addWidget(card, 2);
        }

        root->addWidget(row, 3);
    }

    // ── 하단: 연료 · 수온 · 유압 ─────────────────────────────────────────────
    {
        auto *row = new QWidget(this);
        auto *hb  = new QHBoxLayout(row);
        hb->setContentsMargins(0, 0, 0, 0);
        hb->setSpacing(8);

        // 연료 카드
        {
            auto *card = makeCard(row);
            auto *vb   = new QVBoxLayout(card);
            vb->setContentsMargins(14, 10, 14, 10);
            vb->setSpacing(4);
            vb->addWidget(makeCaption("FUEL", card));
            lbl_fuel_val_ = makeValueLabel("0%", 14, card);
            vb->addWidget(lbl_fuel_val_);

            // 게이지 트랙
            fuel_track_ = new QWidget(card);
            fuel_track_->setFixedHeight(6);
            fuel_track_->setStyleSheet("background: #1a1a1a; border-radius: 3px; border: none;");
            bar_fuel_ = new QWidget(fuel_track_);
            bar_fuel_->setGeometry(0, 0, 0, 6);
            bar_fuel_->setStyleSheet("background: #0fa336; border-radius: 3px; border: none;");
            vb->addWidget(fuel_track_);
            vb->addStretch();
            hb->addWidget(card, 2);
        }

        // 수온 카드
        {
            auto *card = makeCard(row);
            auto *vb   = new QVBoxLayout(card);
            vb->setContentsMargins(14, 10, 14, 10);
            vb->setSpacing(4);
            vb->addWidget(makeCaption("COOLANT", card));
            lbl_coolant_val_ = makeValueLabel("0°C", 14, card);
            vb->addWidget(lbl_coolant_val_);

            coolant_track_ = new QWidget(card);
            coolant_track_->setFixedHeight(6);
            coolant_track_->setStyleSheet("background: #1a1a1a; border-radius: 3px; border: none;");
            bar_coolant_ = new QWidget(coolant_track_);
            bar_coolant_->setGeometry(0, 0, 0, 6);
            bar_coolant_->setStyleSheet("background: #1c69d4; border-radius: 3px; border: none;");
            vb->addWidget(coolant_track_);
            vb->addStretch();
            hb->addWidget(card, 2);
        }

        // 유압 카드
        {
            auto *card = makeCard(row);
            auto *vb   = new QVBoxLayout(card);
            vb->setContentsMargins(14, 10, 14, 10);
            vb->setSpacing(2);
            vb->addWidget(makeCaption("OIL", card));
            lbl_oil_val_ = makeValueLabel("0%", 14, card);
            vb->addWidget(lbl_oil_val_);
            vb->addStretch();
            hb->addWidget(card, 2);
        }

        root->addWidget(row, 2);
    }
}

// ── 슬롯 ─────────────────────────────────────────────────────────────────────

void VehicleInfoWidget::onSpeedChanged(int kmh) {
    speed_ = kmh;
    if (!lbl_speed_val_) return;
    lbl_speed_val_->setText(QString::number(kmh));
    QString col;
    if      (kmh < 80)  col = "#ffffff";
    else if (kmh < 130) col = "#f4b400";
    else                col = "#e22718";
    lbl_speed_val_->setStyleSheet(QStringLiteral("color: %1; background: transparent; border: none;").arg(col));
}

void VehicleInfoWidget::onRpmChanged(int rpm) {
    rpm_ = rpm;
    if (!lbl_rpm_val_) return;
    lbl_rpm_val_->setText(QString::number(rpm));
    // 레드존 6000+
    QString col = (rpm >= 6000) ? "#e22718" : (rpm >= 4500 ? "#f4b400" : "#ffffff");
    lbl_rpm_val_->setStyleSheet(QStringLiteral("color: %1; background: transparent; border: none;").arg(col));
}

void VehicleInfoWidget::onGearChanged(int gear) {
    gear_ = gear;
    if (!lbl_gear_val_) return;
    QString txt;
    if      (gear == 0)    txt = "N";
    else if (gear == 0xFF || gear == -1) txt = "R";
    else                   txt = QString::number(gear);
    lbl_gear_val_->setText(txt);
}

void VehicleInfoWidget::onEngineStateChanged(int coolant, int oilPct, int fuelPct) {
    coolant_  = coolant;
    oil_pct_  = oilPct;
    fuel_pct_ = fuelPct;

    if (lbl_fuel_val_) {
        lbl_fuel_val_->setText(QStringLiteral("%1%").arg(fuelPct));
        QString col = (fuelPct <= 15) ? "#e22718" : (fuelPct <= 30 ? "#f4b400" : "#ffffff");
        lbl_fuel_val_->setStyleSheet(QStringLiteral("color: %1; background: transparent; border: none;").arg(col));
    }
    if (lbl_coolant_val_) {
        lbl_coolant_val_->setText(QStringLiteral("%1°C").arg(coolant));
        QString col = (coolant >= 100) ? "#e22718" : "#ffffff";
        lbl_coolant_val_->setStyleSheet(QStringLiteral("color: %1; background: transparent; border: none;").arg(col));
    }
    if (lbl_oil_val_) {
        lbl_oil_val_->setText(QStringLiteral("%1%").arg(oilPct));
        QString col = (oilPct < 20) ? "#e22718" : "#ffffff";
        lbl_oil_val_->setStyleSheet(QStringLiteral("color: %1; background: transparent; border: none;").arg(col));
    }

    // 게이지 바 업데이트 (resizeEvent 없이 직접 비율 계산)
    if (fuel_track_ && bar_fuel_) {
        int w = fuel_track_->width();
        bar_fuel_->setGeometry(0, 0, w * fuelPct / 100, 6);
        // 연료 부족이면 빨간색
        QString barCol = (fuelPct <= 15) ? "#e22718" : (fuelPct <= 30 ? "#f4b400" : "#0fa336");
        bar_fuel_->setStyleSheet(QStringLiteral("background: %1; border-radius: 3px; border: none;").arg(barCol));
    }
    if (coolant_track_ && bar_coolant_) {
        int w = coolant_track_->width();
        // 수온 0~120°C 범위를 0~100%로 표현
        int pct = qMin(100, coolant * 100 / 120);
        bar_coolant_->setGeometry(0, 0, w * pct / 100, 6);
        QString barCol = (coolant >= 100) ? "#e22718" : "#1c69d4";
        bar_coolant_->setStyleSheet(QStringLiteral("background: %1; border-radius: 3px; border: none;").arg(barCol));
    }
}

void VehicleInfoWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), kBg);
}
