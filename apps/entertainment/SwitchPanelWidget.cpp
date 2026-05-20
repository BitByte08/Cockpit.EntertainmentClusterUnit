#include "SwitchPanelWidget.hpp"
#include "models/EntertainmentModel.hpp"
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFont>

// ── 0x300 비트 정의 ──────────────────────────────────────────────────────────
namespace SB {
    constexpr uint16_t Ignition  = 1 << 0;
    constexpr uint16_t Engine    = 1 << 1;
    constexpr uint16_t HeadLight = 1 << 2;
    constexpr uint16_t HighBeam  = 1 << 3;
    constexpr uint16_t Hazard    = 1 << 4;
    constexpr uint16_t WiperSlow = 1 << 5;
    constexpr uint16_t WiperFast = 1 << 6;
    constexpr uint16_t Horn      = 1 << 7;
}

// ── 0x101 비트 정의 ──────────────────────────────────────────────────────────
namespace TB {
    constexpr uint16_t TurnLeft  = 1 << 8;
    constexpr uint16_t TurnRight = 1 << 9;
}

// ── 스타일 헬퍼 ──────────────────────────────────────────────────────────────
static const char *kInactive =
    "QPushButton { background: #111111; border: 1px solid #2a2a2a; border-radius: 4px;"
    " color: #4a4a4a; font-weight: bold; }"
    "QPushButton:pressed { background: #1c1c1c; }";

static const char *kDisabled =
    "QPushButton { background: #080808; border: 1px solid #181818; border-radius: 4px;"
    " color: #222222; font-weight: bold; }";

static QString activeStyle(const char *bg, const char *border,
                            const char *color, const char *pressed) {
    return QStringLiteral(
        "QPushButton { background: %1; border: 1px solid %2; border-radius: 4px;"
        " color: %3; font-weight: bold; }"
        "QPushButton:pressed { background: %4; }")
        .arg(bg, border, color, pressed);
}

static QString blueStyle()  { return activeStyle("#0d2847","#1c69d4","#ffffff","#1a3a5c"); }
static QString greenStyle() { return activeStyle("#082212","#0fa336","#0fa336","#0e2e18"); }
static QString amberStyle() { return activeStyle("#2a1e00","#f4b400","#f4b400","#3a2a00"); }
static QString redStyle()   { return activeStyle("#2a0808","#e22718","#ff5533","#3a1010"); }

static QString indicStyle(bool active) {
    if (active)
        return "QLabel { background: #2a1e00; border: 1px solid #f4b400;"
               " border-radius: 4px; color: #f4b400; font-weight: bold; }";
    return "QLabel { background: #0a0a0a; border: 1px solid #222222;"
           " border-radius: 4px; color: #2a2a2a; font-weight: bold; }";
}

// ═══════════════════════════════════════════════════════════════════════════════
// SwitchPanelWidget
// ═══════════════════════════════════════════════════════════════════════════════

SwitchPanelWidget::SwitchPanelWidget(QWidget *parent) : QWidget(parent) {
    buildUI();
}

void SwitchPanelWidget::setModel(EntertainmentModel *model) {
    model_ = model;
    connect(model_, &EntertainmentModel::switchFlagsChanged,
            this, &SwitchPanelWidget::onSwitchFlagsChanged);
    connect(model_, &EntertainmentModel::turnFlagsChanged,
            this, &SwitchPanelWidget::onTurnFlagsChanged);
    connect(model_, &EntertainmentModel::rpmChanged,
            this, &SwitchPanelWidget::onRpmChanged);
    onSwitchFlagsChanged(model_->switchFlags());
    onTurnFlagsChanged(model_->turnFlags());
    onRpmChanged(model_->rpm());
}

void SwitchPanelWidget::buildUI() {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 12, 16, 12);
    root->setSpacing(8);

    // ── 헤더 ─────────────────────────────────────────────────────────────────
    {
        auto *header = new QWidget(this);
        header->setFixedHeight(34);
        auto *hb = new QHBoxLayout(header);
        hb->setContentsMargins(0, 0, 0, 0);

        auto *title = new QLabel("VEHICLE CONTROLS", header);
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

    // ── 구분선 ────────────────────────────────────────────────────────────────
    {
        auto *sep = new QWidget(this);
        sep->setFixedHeight(1);
        sep->setStyleSheet("background: #262626;");
        root->addWidget(sep);
    }

    // ── 버튼 그리드 ──────────────────────────────────────────────────────────
    // Row 0: IGNITION | ENGINE  | HEADLIGHT | HIGH BEAM   (4 cols)
    // Row 1: HAZARD   | WIPER (colspan 2)  | HORN         (4 cols)
    auto makeBtn = [&](const QString &label) -> QPushButton * {
        auto *btn = new QPushButton(label, this);
        QFont f; f.setPointSize(8); f.setBold(true);
        btn->setFont(f);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        return btn;
    };

    auto *grid_w = new QWidget(this);
    auto *gl = new QGridLayout(grid_w);
    gl->setSpacing(6);
    gl->setContentsMargins(0, 0, 0, 0);

    // Row 0: START/STOP (colspan 2) | HEAD LIGHT | HIGH BEAM
    // Row 1: HAZARD              | WIPER (colspan 2) | HORN
    btn_start_     = makeBtn("START");
    btn_headlight_ = makeBtn("HEAD\n\nLIGHT");
    btn_highbeam_  = makeBtn("HIGH\n\nBEAM");
    btn_hazard_    = makeBtn("HAZARD\n\n!!!");
    btn_wiper_     = makeBtn("WIPER\n\nOFF");
    btn_horn_      = makeBtn("HORN\n\n---");

    gl->addWidget(btn_start_,     0, 0, 1, 2);  // colspan 2
    gl->addWidget(btn_headlight_, 0, 2);
    gl->addWidget(btn_highbeam_,  0, 3);
    gl->addWidget(btn_hazard_,    1, 0);
    gl->addWidget(btn_wiper_,     1, 1, 1, 2);  // colspan 2
    gl->addWidget(btn_horn_,      1, 3);

    root->addWidget(grid_w, 1);

    // ── 방향지시등 표시 (0x101 스티어링 컬럼, 조작 불가) ─────────────────────
    {
        auto *turn_row = new QWidget(this);
        turn_row->setFixedHeight(58);
        auto *hb = new QHBoxLayout(turn_row);
        hb->setContentsMargins(0, 0, 0, 0);
        hb->setSpacing(8);

        lbl_turn_left_ = new QLabel("  LEFT TURN", turn_row);
        lbl_turn_left_->setAlignment(Qt::AlignCenter);
        QFont f; f.setPointSize(9); f.setBold(true);
        lbl_turn_left_->setFont(f);
        lbl_turn_left_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto *mid = new QLabel("STEERING COLUMN", turn_row);
        mid->setAlignment(Qt::AlignCenter);
        QFont fm; fm.setPointSize(7);
        mid->setFont(fm);
        mid->setStyleSheet("color: #2a2a2a; background: transparent;");
        mid->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

        lbl_turn_right_ = new QLabel("RIGHT TURN  ", turn_row);
        lbl_turn_right_->setAlignment(Qt::AlignCenter);
        lbl_turn_right_->setFont(f);
        lbl_turn_right_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        hb->addWidget(lbl_turn_left_,  2);
        hb->addWidget(mid,             1);
        hb->addWidget(lbl_turn_right_, 2);

        root->addWidget(turn_row);
    }

    refreshStyles();

    // ── 버튼 연결 ─────────────────────────────────────────────────────────────

    // START/STOP 통합 버튼 (모멘터리)
    // - 엔진 꺼짐: Ignition=1 + Engine 펄스 → Unity: OFF→ACC→ON→시동
    // - 엔진 켜짐: Ignition=0 + Engine 펄스 → Unity: 시동 OFF
    connect(btn_start_, &QPushButton::pressed, this, [this] {
        const bool running = (rpm_ > 200);
        setBit(SB::Ignition, !running);
        setBit(SB::Engine, true);
        sendFlags();
    });
    connect(btn_start_, &QPushButton::released, this, [this] {
        setBit(SB::Engine, false);
        sendFlags();
    });

    // HeadLight: 토글. 끄면 HighBeam도 끔
    connect(btn_headlight_, &QPushButton::clicked, this, [this] {
        toggleBit(SB::HeadLight);
        if (!(sw_flags_ & SB::HeadLight))
            setBit(SB::HighBeam, false);
        sendFlags();
    });

    // HighBeam: HeadLight ON일 때만 (토글)
    connect(btn_highbeam_, &QPushButton::clicked, this, [this] {
        if (sw_flags_ & SB::HeadLight) {
            toggleBit(SB::HighBeam);
            sendFlags();
        }
    });

    // Hazard: 토글
    connect(btn_hazard_, &QPushButton::clicked, this, [this] {
        toggleBit(SB::Hazard);
        sendFlags();
    });

    // Wiper: OFF → SLOW → FAST → OFF 순환
    connect(btn_wiper_, &QPushButton::clicked, this, [this] {
        const bool slow = sw_flags_ & SB::WiperSlow;
        const bool fast = sw_flags_ & SB::WiperFast;
        if (!slow && !fast) {
            // OFF → SLOW
            setBit(SB::WiperSlow, true);
            setBit(SB::WiperFast, false);
        } else if (slow && !fast) {
            // SLOW → FAST
            setBit(SB::WiperSlow, false);
            setBit(SB::WiperFast, true);
        } else {
            // FAST → OFF
            setBit(SB::WiperSlow, false);
            setBit(SB::WiperFast, false);
        }
        sendFlags();
    });

    // Horn: 모멘터리 (누르는 동안만 ON)
    connect(btn_horn_, &QPushButton::pressed, this, [this] {
        setBit(SB::Horn, true);
        sendFlags();
    });
    connect(btn_horn_, &QPushButton::released, this, [this] {
        setBit(SB::Horn, false);
        sendFlags();
    });
}

// ── 비트 조작 ─────────────────────────────────────────────────────────────────

void SwitchPanelWidget::toggleBit(uint16_t mask) {
    sw_flags_ ^= mask;
}

void SwitchPanelWidget::setBit(uint16_t mask, bool on) {
    if (on) sw_flags_ |= mask;
    else    sw_flags_ &= ~mask;
}

void SwitchPanelWidget::sendFlags() {
    refreshStyles();
    if (model_) model_->sendSwitchFlags(sw_flags_);
}

// ── 슬롯 ─────────────────────────────────────────────────────────────────────

void SwitchPanelWidget::onSwitchFlagsChanged(uint16_t flags) {
    if (sw_flags_ != flags) {
        sw_flags_ = flags;
        refreshStyles();
    }
}

void SwitchPanelWidget::onTurnFlagsChanged(uint16_t flags) {
    turn_flags_ = flags;
    refreshStyles();
}

void SwitchPanelWidget::onRpmChanged(int rpm) {
    rpm_ = rpm;
    refreshStyles();
}

// ── 스타일 갱신 ──────────────────────────────────────────────────────────────

void SwitchPanelWidget::refreshStyles() {
    const bool ign  = sw_flags_ & SB::Ignition;
    const bool eng  = sw_flags_ & SB::Engine;
    const bool hl   = sw_flags_ & SB::HeadLight;
    const bool hb   = sw_flags_ & SB::HighBeam;
    const bool hz   = sw_flags_ & SB::Hazard;
    const bool slow = sw_flags_ & SB::WiperSlow;
    const bool fast = sw_flags_ & SB::WiperFast;
    const bool hn   = sw_flags_ & SB::Horn;
    const bool tl   = turn_flags_ & TB::TurnLeft;
    const bool tr   = turn_flags_ & TB::TurnRight;

    // 통합 START/STOP 버튼
    const bool running = (rpm_ > 200);
    if (running) {
        btn_start_->setText("ENGINE\nRUNNING\n● STOP");
        btn_start_->setStyleSheet(greenStyle());
    } else if (ign) {
        btn_start_->setText("IGNITION ON\n\n▶ START");
        btn_start_->setStyleSheet(
            activeStyle("#0d1a2e", "#1c69d4", "#7eb8ff", "#0a1220"));
    } else {
        btn_start_->setText("START\n\n○ OFF");
        btn_start_->setStyleSheet(kInactive);
    }

    btn_headlight_->setStyleSheet(hl ? blueStyle() : kInactive);
    btn_highbeam_->setStyleSheet(!hl ? kDisabled : (hb ? blueStyle() : kInactive));

    btn_hazard_->setStyleSheet(hz ? amberStyle() : kInactive);

    // Wiper: 단계별 텍스트 + 색상
    if (fast) {
        btn_wiper_->setText("WIPER\n\nFAST");
        btn_wiper_->setStyleSheet(blueStyle());
    } else if (slow) {
        btn_wiper_->setText("WIPER\n\nSLOW");
        btn_wiper_->setStyleSheet(blueStyle());
    } else {
        btn_wiper_->setText("WIPER\n\nOFF");
        btn_wiper_->setStyleSheet(kInactive);
    }

    btn_horn_->setStyleSheet(hn ? redStyle() : kInactive);

    lbl_turn_left_->setStyleSheet(indicStyle(tl));
    lbl_turn_right_->setStyleSheet(indicStyle(tr));
}

void SwitchPanelWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0));
}
