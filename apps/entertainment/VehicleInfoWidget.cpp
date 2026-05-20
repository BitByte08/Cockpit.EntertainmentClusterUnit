#include "VehicleInfoWidget.hpp"
#include "models/EntertainmentModel.hpp"
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFont>

static const QColor kBg   {0x00, 0x00, 0x00};
static const QColor kHair {0x26, 0x26, 0x26};
static const QColor kMBlue{0x1C, 0x69, 0xD4};
static const QColor kMRed {0xE2, 0x27, 0x18};
static const QColor kOK   {0x0F, 0xA3, 0x36};
static const QColor kWarn {0xF4, 0xB4, 0x00};
static const QColor kFg1  {0xFF, 0xFF, 0xFF};
static const QColor kFg3  {0x7E, 0x7E, 0x7E};

// ── 공통 헬퍼 ─────────────────────────────────────────────────────────────────

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

// ═══════════════════════════════════════════════════════════════════════════════
// VehicleInfoWidget
// ═══════════════════════════════════════════════════════════════════════════════

VehicleInfoWidget::VehicleInfoWidget(QWidget *parent) : QWidget(parent) {
    buildUI();
}

void VehicleInfoWidget::setModel(EntertainmentModel *model) {
    model_ = model;
    connect(model_, &EntertainmentModel::drivingDynamicsChanged,
            this, &VehicleInfoWidget::onDrivingDynamicsChanged);
    connect(model_, &EntertainmentModel::adasStatusChanged,
            this, &VehicleInfoWidget::onAdasStatusChanged);
    connect(model_, &EntertainmentModel::rpmChanged,
            this, &VehicleInfoWidget::onRpmChanged);

    onDrivingDynamicsChanged(model_->transmittedTorque(), model_->lateralG(), model_->longitudinalG());
    onAdasStatusChanged(model_->absActive(), model_->tcsActive(), model_->wheelLockBits());
    onRpmChanged(model_->rpm());
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
        auto *title = new QLabel("DRIVING DYNAMICS", header);
        QFont f; f.setPointSize(10); f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
        title->setFont(f); title->setStyleSheet("color: #ffffff;");
        hb->addWidget(title); hb->addStretch();
        auto *stripe = new QWidget(header);
        stripe->setFixedSize(20, 24);
        stripe->setStyleSheet(
            "background: qlineargradient(x1:0,y1:0,x2:0,y2:1,"
            "stop:0 #0066b1,stop:0.333 #0066b1,"
            "stop:0.334 #1c69d4,stop:0.666 #1c69d4,"
            "stop:0.667 #e22718,stop:1 #e22718);");
        hb->addWidget(stripe);
        root->addWidget(header);
    }
    { auto *sep = new QWidget(this); sep->setFixedHeight(1);
      sep->setStyleSheet("background: #262626;"); root->addWidget(sep); }

    // ── 상단: 전달 토크 · 횡G · 종G ──────────────────────────────────────────
    {
        auto *row = new QWidget(this);
        auto *hb  = new QHBoxLayout(row);
        hb->setContentsMargins(0, 0, 0, 0); hb->setSpacing(8);

        // 전달 토크
        {
            auto *card = makeCard(row);
            auto *vb   = new QVBoxLayout(card);
            vb->setContentsMargins(14, 10, 14, 10); vb->setSpacing(2);
            vb->addWidget(makeCaption("TORQUE", card));
            auto *numRow = new QHBoxLayout();
            numRow->setSpacing(4);
            lbl_torque_ = makeValueLabel("0", 20, card);
            numRow->addWidget(lbl_torque_);
            auto *unit = new QLabel("Nm", card);
            QFont fu; fu.setPointSize(7); fu.setBold(true);
            unit->setFont(fu);
            unit->setStyleSheet("color: #7e7e7e; background: transparent; border: none;");
            unit->setAlignment(Qt::AlignLeft | Qt::AlignBottom);
            numRow->addWidget(unit); numRow->addStretch();
            vb->addLayout(numRow);
            hb->addWidget(card, 2);
        }

        // 횡G
        {
            auto *card = makeCard(row);
            auto *vb   = new QVBoxLayout(card);
            vb->setContentsMargins(14, 10, 14, 10); vb->setSpacing(4);
            vb->addWidget(makeCaption("LAT G", card));
            lbl_lat_g_ = makeValueLabel("0.00 g", 14, card);
            vb->addWidget(lbl_lat_g_);

            track_lat_ = new QWidget(card);
            track_lat_->setFixedHeight(6);
            track_lat_->setStyleSheet("background: #1a1a1a; border-radius: 3px; border: none;");
            // 중심 기준 양쪽 방향 바
            bar_lat_pos_ = new QWidget(track_lat_);
            bar_lat_pos_->setStyleSheet("background: #e22718; border-radius: 3px; border: none;");
            bar_lat_neg_ = new QWidget(track_lat_);
            bar_lat_neg_->setStyleSheet("background: #1c69d4; border-radius: 3px; border: none;");
            vb->addWidget(track_lat_);
            vb->addStretch();
            hb->addWidget(card, 2);
        }

        // 종G
        {
            auto *card = makeCard(row);
            auto *vb   = new QVBoxLayout(card);
            vb->setContentsMargins(14, 10, 14, 10); vb->setSpacing(4);
            vb->addWidget(makeCaption("LON G", card));
            lbl_lon_g_ = makeValueLabel("0.00 g", 14, card);
            vb->addWidget(lbl_lon_g_);

            track_lon_ = new QWidget(card);
            track_lon_->setFixedHeight(6);
            track_lon_->setStyleSheet("background: #1a1a1a; border-radius: 3px; border: none;");
            bar_lon_pos_ = new QWidget(track_lon_);
            bar_lon_pos_->setStyleSheet("background: #0fa336; border-radius: 3px; border: none;");
            bar_lon_neg_ = new QWidget(track_lon_);
            bar_lon_neg_->setStyleSheet("background: #f4b400; border-radius: 3px; border: none;");
            vb->addWidget(track_lon_);
            vb->addStretch();
            hb->addWidget(card, 2);
        }

        root->addWidget(row, 2);
    }

    // ── 하단: ABS/TCS 상태 + 휠락 ────────────────────────────────────────────
    {
        auto *row = new QWidget(this);
        auto *hb  = new QHBoxLayout(row);
        hb->setContentsMargins(0, 0, 0, 0); hb->setSpacing(8);

        // ABS / TCS 상태 카드
        {
            auto *card = makeCard(row);
            auto *vb   = new QVBoxLayout(card);
            vb->setContentsMargins(14, 10, 14, 10); vb->setSpacing(6);
            vb->addWidget(makeCaption("ADAS STATUS", card));

            auto makeStatus = [&](const QString &label) -> QLabel * {
                auto *lbl = new QLabel(label, card);
                QFont f; f.setPointSize(9); f.setBold(true);
                f.setLetterSpacing(QFont::AbsoluteSpacing, 1);
                lbl->setFont(f);
                lbl->setAlignment(Qt::AlignCenter);
                lbl->setStyleSheet(
                    "color: #2a2a2a; background: #111111;"
                    "border: 1px solid #1e1e1e; border-radius: 4px; padding: 4px;");
                return lbl;
            };
            lbl_abs_ = makeStatus("ABS");
            lbl_tcs_ = makeStatus("TCS");
            vb->addWidget(lbl_abs_);
            vb->addWidget(lbl_tcs_);
            vb->addStretch();
            hb->addWidget(card, 1);
        }

        // 휠락 인디케이터 카드 (자동차 top-view 배치)
        {
            auto *card = makeCard(row);
            auto *vb   = new QVBoxLayout(card);
            vb->setContentsMargins(14, 10, 14, 10); vb->setSpacing(6);
            vb->addWidget(makeCaption("WHEEL LOCK", card));

            auto makeInd = [&](const QString &label) -> QWidget * {
                auto *w  = new QWidget(card);
                auto *vb2 = new QVBoxLayout(w);
                vb2->setContentsMargins(0, 0, 0, 0); vb2->setSpacing(2);
                auto *lbl = new QLabel(label, w);
                QFont f; f.setPointSize(6); f.setBold(true);
                lbl->setFont(f);
                lbl->setAlignment(Qt::AlignCenter);
                lbl->setStyleSheet("color: #4a4a4a; background: transparent; border: none;");
                // 인디케이터 사각형
                auto *box = new QWidget(w);
                box->setFixedHeight(20);
                box->setObjectName("indicator");
                box->setStyleSheet("background: #111111; border: 1px solid #1e1e1e; border-radius: 3px;");
                vb2->addWidget(lbl);
                vb2->addWidget(box);
                return w;
            };

            ind_fl_ = makeInd("FL");
            ind_fr_ = makeInd("FR");
            ind_rl_ = makeInd("RL");
            ind_rr_ = makeInd("RR");

            // 2×2 그리드 (앞 / 뒤)
            auto *grid_w = new QWidget(card);
            auto *gl = new QGridLayout(grid_w);
            gl->setSpacing(6); gl->setContentsMargins(0, 0, 0, 0);
            gl->addWidget(ind_fl_, 0, 0);
            gl->addWidget(ind_fr_, 0, 1);
            gl->addWidget(ind_rl_, 1, 0);
            gl->addWidget(ind_rr_, 1, 1);
            vb->addWidget(grid_w, 1);
            hb->addWidget(card, 2);
        }

        root->addWidget(row, 3);
    }
}

// ── 슬롯 ─────────────────────────────────────────────────────────────────────

void VehicleInfoWidget::onDrivingDynamicsChanged(int torque, double latG, double lonG) {
    torque_ = torque; lat_g_ = latG; lon_g_ = lonG;

    if (lbl_torque_) {
        lbl_torque_->setText(QString::number(torque));
        QString col = (torque > 100) ? "#0fa336" : (torque < -50) ? "#f4b400" : "#ffffff";
        lbl_torque_->setStyleSheet(QStringLiteral("color: %1; background: transparent; border: none;").arg(col));
    }
    if (lbl_lat_g_) {
        lbl_lat_g_->setText(QStringLiteral("%1 g").arg(latG, 0, 'f', 2));
        QString col = (qAbs(latG) > 0.6) ? "#e22718" : (qAbs(latG) > 0.3) ? "#f4b400" : "#ffffff";
        lbl_lat_g_->setStyleSheet(QStringLiteral("color: %1; background: transparent; border: none;").arg(col));
    }
    if (lbl_lon_g_) {
        lbl_lon_g_->setText(QStringLiteral("%1 g").arg(lonG, 0, 'f', 2));
        QString col = (lonG < -0.5) ? "#f4b400" : (lonG > 0.4) ? "#0fa336" : "#ffffff";
        lbl_lon_g_->setStyleSheet(QStringLiteral("color: %1; background: transparent; border: none;").arg(col));
    }

    // G-force 바 업데이트 (resizeEvent 없이 직접)
    if (track_lat_) {
        int halfW = track_lat_->width() / 2;
        int pxL = qMin((int)(qAbs(latG < 0 ? latG : 0) / 1.2 * halfW), halfW);
        int pxR = qMin((int)(qAbs(latG > 0 ? latG : 0) / 1.2 * halfW), halfW);
        if (bar_lat_neg_) bar_lat_neg_->setGeometry(halfW - pxL, 0, pxL, 6);
        if (bar_lat_pos_) bar_lat_pos_->setGeometry(halfW, 0, pxR, 6);
    }
    if (track_lon_) {
        int halfW = track_lon_->width() / 2;
        int pxA = qMin((int)(qAbs(lonG > 0 ? lonG : 0) / 1.0 * halfW), halfW);
        int pxB = qMin((int)(qAbs(lonG < 0 ? lonG : 0) / 1.0 * halfW), halfW);
        if (bar_lon_pos_) bar_lon_pos_->setGeometry(halfW, 0, pxA, 6);
        if (bar_lon_neg_) bar_lon_neg_->setGeometry(halfW - pxB, 0, pxB, 6);
    }
}

void VehicleInfoWidget::onAdasStatusChanged(bool absActive, bool tcsActive, uint8_t wheelLock) {
    abs_active_ = absActive; tcs_active_ = tcsActive; wheel_lock_ = wheelLock;

    auto setStatus = [](QLabel *lbl, bool active, const char *onCol) {
        if (!lbl) return;
        if (active)
            lbl->setStyleSheet(QStringLiteral(
                "color: #000000; background: %1; border: 1px solid %1;"
                " border-radius: 4px; padding: 4px;").arg(onCol));
        else
            lbl->setStyleSheet(
                "color: #2a2a2a; background: #111111;"
                "border: 1px solid #1e1e1e; border-radius: 4px; padding: 4px;");
    };

    setStatus(lbl_abs_, absActive, "#f4b400");
    setStatus(lbl_tcs_, tcsActive, "#1c69d4");

    setWheelLock(ind_fl_, wheelLock & 0x01);
    setWheelLock(ind_fr_, wheelLock & 0x02);
    setWheelLock(ind_rl_, wheelLock & 0x04);
    setWheelLock(ind_rr_, wheelLock & 0x08);
}

void VehicleInfoWidget::onRpmChanged(int rpm) {
    rpm_ = rpm;
}

void VehicleInfoWidget::setWheelLock(QWidget *ind, bool locked) {
    if (!ind) return;
    auto *box = ind->findChild<QWidget *>("indicator");
    if (!box) return;
    box->setStyleSheet(locked
        ? "background: #e22718; border: 1px solid #ff4433; border-radius: 3px;"
        : "background: #111111; border: 1px solid #1e1e1e; border-radius: 3px;");
}

void VehicleInfoWidget::updateGBar(QWidget *posBar, QWidget *negBar,
                                   QWidget *track, double val, double maxVal) {
    if (!track) return;
    int halfW = track->width() / 2;
    int pxPos = qMin((int)(qMax(0.0,  val) / maxVal * halfW), halfW);
    int pxNeg = qMin((int)(qMax(0.0, -val) / maxVal * halfW), halfW);
    if (posBar) posBar->setGeometry(halfW, 0, pxPos, 6);
    if (negBar) negBar->setGeometry(halfW - pxNeg, 0, pxNeg, 6);
}

void VehicleInfoWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), kBg);
}
