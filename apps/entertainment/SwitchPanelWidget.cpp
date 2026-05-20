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
        hb->addWidget(title); hb->addStretch();
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
    { auto *sep = new QWidget(this); sep->setFixedHeight(1);
      sep->setStyleSheet("background: #262626;"); root->addWidget(sep); }

    // ── 버튼 그리드 ──────────────────────────────────────────────────────────
    // Row 0: START/STOP (colspan 2) | LIGHTS (cycling) | WIPER (cycling)
    // Row 1: TURN LEFT | HAZARD | TURN RIGHT | HORN
    auto makeBtn = [&](const QString &label) -> QPushButton * {
        auto *btn = new QPushButton(label, this);
        QFont f; f.setPointSize(8); f.setBold(true);
        btn->setFont(f);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        return btn;
    };

    auto *grid_w = new QWidget(this);
    auto *gl = new QGridLayout(grid_w);
    gl->setSpacing(6); gl->setContentsMargins(0, 0, 0, 0);

    btn_start_      = makeBtn("START");
    btn_lights_     = makeBtn("LIGHTS\n\nOFF");
    btn_wiper_      = makeBtn("WIPER\n\nOFF");
    btn_turn_left_  = makeBtn("◀  LEFT");
    btn_hazard_     = makeBtn("HAZARD\n!!!");
    btn_turn_right_ = makeBtn("RIGHT  ▶");
    btn_horn_       = makeBtn("HORN");

    gl->addWidget(btn_start_,      0, 0, 1, 2);  // row0, col0, colspan2
    gl->addWidget(btn_lights_,     0, 2);
    gl->addWidget(btn_wiper_,      0, 3);
    gl->addWidget(btn_turn_left_,  1, 0);
    gl->addWidget(btn_hazard_,     1, 1);
    gl->addWidget(btn_turn_right_, 1, 2);
    gl->addWidget(btn_horn_,       1, 3);

    root->addWidget(grid_w, 1);

    refreshStyles();

    // ── 버튼 연결 ─────────────────────────────────────────────────────────────

    // START/STOP 통합 (모멘터리)
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

    // LIGHTS: OFF → LOW → HIGH → OFF 순환
    connect(btn_lights_, &QPushButton::clicked, this, [this] {
        const bool lo = sw_flags_ & SB::HeadLight;
        const bool hi = sw_flags_ & SB::HighBeam;
        if (!lo && !hi) {
            // OFF → LOW
            setBit(SB::HeadLight, true);
            setBit(SB::HighBeam,  false);
        } else if (lo && !hi) {
            // LOW → HIGH
            setBit(SB::HighBeam, true);
        } else {
            // HIGH → OFF
            setBit(SB::HeadLight, false);
            setBit(SB::HighBeam,  false);
        }
        sendFlags();
    });

    // WIPER: OFF → SLOW → FAST → OFF 순환
    connect(btn_wiper_, &QPushButton::clicked, this, [this] {
        const bool slow = sw_flags_ & SB::WiperSlow;
        const bool fast = sw_flags_ & SB::WiperFast;
        if (!slow && !fast) {
            setBit(SB::WiperSlow, true); setBit(SB::WiperFast, false);
        } else if (slow) {
            setBit(SB::WiperSlow, false); setBit(SB::WiperFast, true);
        } else {
            setBit(SB::WiperSlow, false); setBit(SB::WiperFast, false);
        }
        sendFlags();
    });

    // 좌 방향지시등: 토글 (TurnLeft 비트, 0x300 bit8)
    connect(btn_turn_left_, &QPushButton::clicked, this, [this] {
        toggleBit(SB::TurnLeft);
        if (sw_flags_ & SB::TurnLeft) setBit(SB::TurnRight, false); // 상호 배타
        sendFlags();
    });

    // 비상등: 토글
    connect(btn_hazard_, &QPushButton::clicked, this, [this] {
        toggleBit(SB::Hazard);
        if (sw_flags_ & SB::Hazard) {
            setBit(SB::TurnLeft,  false);
            setBit(SB::TurnRight, false);
        }
        sendFlags();
    });

    // 우 방향지시등: 토글
    connect(btn_turn_right_, &QPushButton::clicked, this, [this] {
        toggleBit(SB::TurnRight);
        if (sw_flags_ & SB::TurnRight) setBit(SB::TurnLeft, false);
        sendFlags();
    });

    // HORN: 모멘터리
    connect(btn_horn_, &QPushButton::pressed,   this, [this]{ setBit(SB::Horn, true);  sendFlags(); });
    connect(btn_horn_, &QPushButton::released,  this, [this]{ setBit(SB::Horn, false); sendFlags(); });
}

// ── 비트 조작 ─────────────────────────────────────────────────────────────────

void SwitchPanelWidget::toggleBit(uint16_t mask) { sw_flags_ ^= mask; }
void SwitchPanelWidget::setBit(uint16_t mask, bool on) {
    if (on) sw_flags_ |= mask; else sw_flags_ &= ~mask;
}
void SwitchPanelWidget::sendFlags() {
    refreshStyles();
    if (model_) model_->sendSwitchFlags(sw_flags_);
}

// ── 슬롯 ─────────────────────────────────────────────────────────────────────

void SwitchPanelWidget::onSwitchFlagsChanged(uint16_t flags) {
    if (sw_flags_ != flags) { sw_flags_ = flags; refreshStyles(); }
}
void SwitchPanelWidget::onTurnFlagsChanged(uint16_t flags) {
    turn_flags_ = flags; refreshStyles();
}
void SwitchPanelWidget::onRpmChanged(int rpm) {
    rpm_ = rpm; refreshStyles();
}

// ── 스타일 갱신 ──────────────────────────────────────────────────────────────

void SwitchPanelWidget::refreshStyles() {
    const bool ign  = sw_flags_ & SB::Ignition;
    const bool lo   = sw_flags_ & SB::HeadLight;
    const bool hi   = sw_flags_ & SB::HighBeam;
    const bool hz   = sw_flags_ & SB::Hazard;
    const bool slow = sw_flags_ & SB::WiperSlow;
    const bool fast = sw_flags_ & SB::WiperFast;
    const bool hn   = sw_flags_ & SB::Horn;
    // 방향지시등: 패널 송신 비트 OR 스티어링 컬럼 수신 비트
    const bool tl   = (sw_flags_ | turn_flags_) & SB::TurnLeft;
    const bool tr   = (sw_flags_ | turn_flags_) & SB::TurnRight;

    // START/STOP
    const bool running = (rpm_ > 200);
    if (running) {
        btn_start_->setText("ENGINE\nRUNNING\n● STOP");
        btn_start_->setStyleSheet(greenStyle());
    } else if (ign) {
        btn_start_->setText("IGNITION ON\n\n▶ START");
        btn_start_->setStyleSheet(activeStyle("#0d1a2e","#1c69d4","#7eb8ff","#0a1220"));
    } else {
        btn_start_->setText("START\n\n○ OFF");
        btn_start_->setStyleSheet(kInactive);
    }

    // LIGHTS: OFF/LOW/HIGH
    if (lo && hi) {
        btn_lights_->setText("LIGHTS\n\nHIGH");
        btn_lights_->setStyleSheet(blueStyle());
    } else if (lo) {
        btn_lights_->setText("LIGHTS\n\nLOW");
        btn_lights_->setStyleSheet(activeStyle("#0d1a2e","#7eb8ff","#7eb8ff","#0a1220"));
    } else {
        btn_lights_->setText("LIGHTS\n\nOFF");
        btn_lights_->setStyleSheet(kInactive);
    }

    // WIPER
    if (fast)      { btn_wiper_->setText("WIPER\n\nFAST"); btn_wiper_->setStyleSheet(blueStyle()); }
    else if (slow) { btn_wiper_->setText("WIPER\n\nSLOW"); btn_wiper_->setStyleSheet(blueStyle()); }
    else           { btn_wiper_->setText("WIPER\n\nOFF");  btn_wiper_->setStyleSheet(kInactive);   }

    // 방향지시등 (버튼화)
    btn_turn_left_ ->setStyleSheet(tl ? amberStyle() : kInactive);
    btn_turn_right_->setStyleSheet(tr ? amberStyle() : kInactive);

    // 비상등
    btn_hazard_->setStyleSheet(hz ? amberStyle() : kInactive);

    // HORN
    btn_horn_->setStyleSheet(hn ? redStyle() : kInactive);
}

void SwitchPanelWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0));
}
