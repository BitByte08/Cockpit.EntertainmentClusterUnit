#include "mainwindow.hpp"
#include "models/ClusterModel.hpp"
#include "widgets/RpmBarWidget.hpp"
#include "widgets/FuelBarWidget.hpp"
#include "widgets/IndicatorWidget.hpp"
#include "UpdateManager.hpp"
#include "StubCANInterface.hpp"
#ifndef _WIN32
#include "SocketCANInterface.hpp"
#endif

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QProcess>
#include <QTimer>
#include <QTime>
#include <QFont>
#include <QFontDatabase>
#include <QApplication>

static constexpr const char *kGithubRepo = "BitByte08/Cockpit.EntertainmentClusterUnit";

// ── BMW M Design System — color constants ─────────────────────────────────────
static const QColor kBgCanvas   {0x00, 0x00, 0x00};   // #000000  window bg
static const QColor kBgSurface  {0x0D, 0x0D, 0x0D};   // #0d0d0d  panel bg
static const QColor kHairline   {0x26, 0x26, 0x26};   // #262626  dividers
static const QColor kFg1        {0xFF, 0xFF, 0xFF};   // #ffffff  primary text
static const QColor kFg3        {0x7E, 0x7E, 0x7E};   // #7e7e7e  muted labels
static const QColor kMBlueDark  {0x1C, 0x69, 0xD4};   // #1c69d4  accent / gear D
static const QColor kMRed       {0xE2, 0x27, 0x18};   // #e22718  error / gear R
static const QColor kWarning    {0xF4, 0xB4, 0x00};   // #f4b400  warn amber
static const QColor kSuccess    {0x0F, 0xA3, 0x36};   // #0fa336  CAN OK

// ── Helpers ───────────────────────────────────────────────────────────────────

static QFrame *makeHLine(QWidget *parent) {
    auto *f = new QFrame(parent);
    f->setFrameShape(QFrame::HLine);
    f->setFixedHeight(1);
    f->setStyleSheet("background-color: #262626; border: none;");
    return f;
}

static QFrame *makeVLine(QWidget *parent) {
    auto *f = new QFrame(parent);
    f->setFrameShape(QFrame::VLine);
    f->setFixedWidth(1);
    f->setStyleSheet("background-color: #262626; border: none;");
    return f;
}

// M tricolor stripe (3 px, sharp stops — BMW M discipline)
static QFrame *makeMStripe(QWidget *parent) {
    auto *f = new QFrame(parent);
    f->setFixedHeight(3);
    f->setStyleSheet(
        "border: none;"
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
        " stop:0 #0066b1, stop:0.3333 #0066b1,"
        " stop:0.3334 #1c69d4, stop:0.6666 #1c69d4,"
        " stop:0.6667 #e22718, stop:1 #e22718);"
    );
    return f;
}

// ── Constructor ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("M Cockpit — Cluster");
    setMinimumSize(800, 480);

    // Global style: pure black bg + Inter font
    setStyleSheet(
        "QMainWindow, QWidget { background-color: #000000; }"
        "QLabel { background: transparent; font-family: 'Inter', 'Noto Sans', sans-serif; }"
    );

    cluster_model_ = new ClusterModel(this);

    setupUI();
    connectSignals();

    // ── CAN interface selection ──────────────────────────────────────────────
#ifdef _WIN32
    cluster_model_->setCANInterface(std::make_unique<StubCANInterface>());
    canStatusLabel_->setText("CAN: stub  ○");
    canStatusLabel_->setStyleSheet("color: #886600; font-size: 11px; font-weight: bold;");
#else
    QString canIf = qEnvironmentVariable("CLUSTER_CAN_IF", "can0");
    bool canOk = false;
    try {
        cluster_model_->setCANInterface(std::make_unique<SocketCANInterface>(canIf.toStdString()));
        canOk = true;
    } catch (const std::exception &) {
        cluster_model_->setCANInterface(std::make_unique<StubCANInterface>());
    }
    if (canOk) {
        canStatusLabel_->setText(QStringLiteral("CAN: %1  ●").arg(canIf));
        canStatusLabel_->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold;")
                                       .arg(kSuccess.name()));
    } else {
        canStatusLabel_->setText("CAN: stub  ○");
        canStatusLabel_->setStyleSheet("color: #886600; font-size: 11px; font-weight: bold;");
    }
#endif

    cluster_model_->startReceiving();

    // ── OTA update manager ──────────────────────────────────────────────────
    update_manager_ = new UpdateManager(APP_VERSION, kGithubRepo, this);
    connect(update_manager_, &UpdateManager::updateAvailable, this, &MainWindow::onUpdateAvailable);
    connect(update_manager_, &UpdateManager::updateProgress,  this, &MainWindow::onUpdateProgress);
    connect(update_manager_, &UpdateManager::updateReady,     this, &MainWindow::onUpdateReady);
    connect(update_manager_, &UpdateManager::updateError,     this, &MainWindow::onUpdateError);
    QTimer::singleShot(5000, update_manager_, &UpdateManager::checkForUpdate);

    // ── Clock ────────────────────────────────────────────────────────────────
    clockTimer_ = new QTimer(this);
    connect(clockTimer_, &QTimer::timeout, this, &MainWindow::updateClock);
    clockTimer_->start(1000);
    updateClock();
}

// ── UI setup ─────────────────────────────────────────────────────────────────

void MainWindow::setupUI() {
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *root = new QVBoxLayout(central);
    root->setSpacing(0);
    root->setContentsMargins(0, 0, 0, 0);

    root->addWidget(buildIndicatorBar());
    root->addWidget(makeMStripe(central));
    root->addWidget(buildRpmBarRow());
    root->addWidget(makeHLine(central));

    // Main 3-column area
    auto *mainArea   = new QWidget(central);
    auto *mainLayout = new QHBoxLayout(mainArea);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(buildLeftPanel());
    mainLayout->addWidget(makeVLine(mainArea));
    mainLayout->addWidget(buildCenterPanel(), 1);
    mainLayout->addWidget(makeVLine(mainArea));
    mainLayout->addWidget(buildRightPanel());

    root->addWidget(mainArea, 1);
    root->addWidget(makeHLine(central));
    root->addWidget(buildStatusBar());
}

// ── Indicator bar (top, 44 px) ────────────────────────────────────────────────

QWidget *MainWindow::buildIndicatorBar() {
    auto *bar = new QWidget;
    bar->setFixedHeight(44);
    bar->setStyleSheet(QString("background-color: %1;").arg(kBgSurface.name()));

    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(16, 6, 16, 6);
    layout->setSpacing(8);

    // Create indicators — BMW M colors
    turnLeftInd_    = new IndicatorWidget(IndicatorIcon::TurnLeft,    "TURN L",  kSuccess,       bar);
    highBeamInd_    = new IndicatorWidget(IndicatorIcon::HighBeam,    "HI BEAM", kMBlueDark,     bar);
    checkEngineInd_ = new IndicatorWidget(IndicatorIcon::CheckEngine, "ENGINE",  kMRed,          bar);
    oilInd_         = new IndicatorWidget(IndicatorIcon::OilPressure, "OIL",     kMRed,          bar);
    absInd_         = new IndicatorWidget(IndicatorIcon::ABS,         "ABS",     kWarning,       bar);
    tcsInd_         = new IndicatorWidget(IndicatorIcon::TCS,         "TCS",     kWarning,       bar);
    fuelWarnInd_    = new IndicatorWidget(IndicatorIcon::FuelWarn,    "FUEL",    kWarning,       bar);
    turnRightInd_   = new IndicatorWidget(IndicatorIcon::TurnRight,   "TURN R",  kSuccess,       bar);

    for (auto *w : {(QWidget*)turnLeftInd_,  (QWidget*)highBeamInd_,
                    (QWidget*)checkEngineInd_,(QWidget*)oilInd_,
                    (QWidget*)absInd_,        (QWidget*)tcsInd_,
                    (QWidget*)fuelWarnInd_,   (QWidget*)turnRightInd_}) {
        w->setFixedSize(48, 32);
    }

    // [TL][BEAM]  ─spacer─  [ENG][OIL][ABS][TCS]  ─spacer─  [FUEL][TR]
    layout->addWidget(turnLeftInd_);
    layout->addWidget(highBeamInd_);
    layout->addStretch(1);
    layout->addWidget(checkEngineInd_);
    layout->addWidget(oilInd_);
    layout->addWidget(absInd_);
    layout->addWidget(tcsInd_);
    layout->addStretch(1);
    layout->addWidget(fuelWarnInd_);
    layout->addWidget(turnRightInd_);

    return bar;
}

// ── RPM bar row (44 px) ───────────────────────────────────────────────────────

QWidget *MainWindow::buildRpmBarRow() {
    auto *row = new QWidget;
    row->setFixedHeight(44);
    row->setStyleSheet(QString("background-color: %1;").arg(kBgCanvas.name()));

    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(16, 6, 16, 6);
    layout->setSpacing(10);

    // "RPM × 1000" label (left)
    auto *rpmCaption = new QLabel("RPM × 1000", row);
    rpmCaption->setStyleSheet(
        QString("color: %1; font-size: 9px; font-weight: bold; letter-spacing: 1.5px;")
        .arg(kFg3.name()));

    // Discrete cell bar (expands)
    rpmBar_ = new RpmBarWidget(row);
    rpmBar_->setMax(8000);
    rpmBar_->setRedline(7000);
    rpmBar_->setCells(48);
    rpmBar_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Numeric RPM readout (right, fixed width so it doesn't jump)
    rpmValueLabel_ = new QLabel("0", row);
    rpmValueLabel_->setFixedWidth(60);
    rpmValueLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    rpmValueLabel_->setStyleSheet(
        "color: #ffffff; font-size: 13px; font-weight: bold;");

    layout->addWidget(rpmCaption);
    layout->addWidget(rpmBar_, 1);
    layout->addWidget(rpmValueLabel_);

    return row;
}

// ── Left panel: gear + fuel (width 280 px) ────────────────────────────────────

QWidget *MainWindow::buildLeftPanel() {
    auto *panel = new QWidget;
    panel->setFixedWidth(280);
    panel->setStyleSheet(QString("background-color: %1;").arg(kBgSurface.name()));

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(0);

    // ── Gear section ─────────────────────────────────────────────────────────
    auto *gearCaption = new QLabel("GEAR", panel);
    gearCaption->setStyleSheet(
        QString("color: %1; font-size: 9px; font-weight: bold; letter-spacing: 1.5px;")
        .arg(kFg3.name()));
    layout->addWidget(gearCaption);

    gearLabel_ = new QLabel("N", panel);
    {
        QFont f;
        f.setPointSize(72);
        f.setBold(true);
        f.setStyleHint(QFont::SansSerif);
        gearLabel_->setFont(f);
    }
    gearLabel_->setStyleSheet("color: #ffffff;");
    layout->addWidget(gearLabel_);

    gearDescLabel_ = new QLabel("NEUTRAL", panel);
    gearDescLabel_->setStyleSheet(
        QString("color: %1; font-size: 9px; font-weight: bold; letter-spacing: 1px;")
        .arg(kFg3.name()));
    layout->addWidget(gearDescLabel_);

    layout->addStretch(1);

    // ── Fuel section ─────────────────────────────────────────────────────────
    auto *fuelTopRow = new QWidget(panel);
    auto *fuelTopLayout = new QHBoxLayout(fuelTopRow);
    fuelTopLayout->setContentsMargins(0, 0, 0, 4);
    fuelTopLayout->setSpacing(0);

    auto *fuelCaption = new QLabel("FUEL", fuelTopRow);
    fuelCaption->setStyleSheet(
        QString("color: %1; font-size: 9px; font-weight: bold; letter-spacing: 1.5px;")
        .arg(kFg3.name()));

    fuelPctLabel_ = new QLabel("100%", fuelTopRow);
    fuelPctLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    fuelPctLabel_->setStyleSheet("color: #ffffff; font-size: 13px; font-weight: bold;");

    fuelTopLayout->addWidget(fuelCaption);
    fuelTopLayout->addStretch(1);
    fuelTopLayout->addWidget(fuelPctLabel_);
    layout->addWidget(fuelTopRow);

    fuelBar_ = new FuelBarWidget(panel);
    fuelBar_->setFixedHeight(8);
    fuelBar_->setPercent(cluster_model_->getFuelLevel());
    layout->addWidget(fuelBar_);

    layout->addSpacing(6);
    return panel;
}

// ── Center panel: speed (flex) ────────────────────────────────────────────────

QWidget *MainWindow::buildCenterPanel() {
    auto *panel = new QWidget;
    panel->setStyleSheet(QString("background-color: %1;").arg(kBgCanvas.name()));

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(0);

    layout->addStretch(3);

    // Giant speed number — fills vertical space, shrinks on small screens
    speedValueLabel_ = new QLabel("0", panel);
    {
        QFont f;
        f.setPointSize(108);
        f.setBold(true);
        f.setStyleHint(QFont::SansSerif);
        f.setLetterSpacing(QFont::AbsoluteSpacing, -3);
        speedValueLabel_->setFont(f);
    }
    speedValueLabel_->setAlignment(Qt::AlignCenter);
    speedValueLabel_->setStyleSheet("color: #ffffff;");
    layout->addWidget(speedValueLabel_);

    // "KM / H" unit label
    auto *unitLabel = new QLabel("KM / H", panel);
    unitLabel->setAlignment(Qt::AlignCenter);
    unitLabel->setStyleSheet(
        QString("color: %1; font-size: 11px; font-weight: bold; letter-spacing: 4px;")
        .arg(kFg3.name()));
    layout->addWidget(unitLabel);

    layout->addStretch(4);

    return panel;
}

// ── Right panel: status + metrics (width 240 px) ──────────────────────────────

QWidget *MainWindow::buildRightPanel() {
    auto *panel = new QWidget;
    panel->setFixedWidth(240);
    panel->setStyleSheet(QString("background-color: %1;").arg(kBgSurface.name()));

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(10);

    // ── Switch status row ────────────────────────────────────────────────────
    ignitionLabel_ = new QLabel("IGN: OFF", panel);
    ignitionLabel_->setStyleSheet(
        "color: #262626; font-size: 11px; font-weight: bold; letter-spacing: 1px;");

    engineLabel_ = new QLabel("ENG: OFF", panel);
    engineLabel_->setStyleSheet(
        "color: #262626; font-size: 11px; font-weight: bold; letter-spacing: 1px;");

    headlightLabel_ = new QLabel("LIGHT: OFF", panel);
    headlightLabel_->setStyleSheet(
        "color: #262626; font-size: 11px; font-weight: bold; letter-spacing: 1px;");

    layout->addWidget(ignitionLabel_);
    layout->addWidget(engineLabel_);
    layout->addWidget(headlightLabel_);
    layout->addWidget(makeHLine(panel));

    // ── Coolant temp metric ──────────────────────────────────────────────────
    auto *tempCaption = new QLabel("COOLANT", panel);
    tempCaption->setStyleSheet(
        QString("color: %1; font-size: 9px; font-weight: bold; letter-spacing: 1.5px;")
        .arg(kFg3.name()));

    tempLabel_ = new QLabel(QString("%1°C").arg(cluster_model_->getTemperature()), panel);
    tempLabel_->setStyleSheet("color: #ffffff; font-size: 26px; font-weight: bold;");

    layout->addWidget(tempCaption);
    layout->addWidget(tempLabel_);
    layout->addWidget(makeHLine(panel));

    // ── Oil pressure metric ──────────────────────────────────────────────────
    auto *oilCaption = new QLabel("OIL PRESS", panel);
    oilCaption->setStyleSheet(
        QString("color: %1; font-size: 9px; font-weight: bold; letter-spacing: 1.5px;")
        .arg(kFg3.name()));

    oilPressureLabel_ = new QLabel(QString::number(cluster_model_->getOilPressure()), panel);
    oilPressureLabel_->setStyleSheet("color: #ffffff; font-size: 26px; font-weight: bold;");

    layout->addWidget(oilCaption);
    layout->addWidget(oilPressureLabel_);

    layout->addStretch(1);

    return panel;
}

// ── Status bar (bottom, 36 px) ────────────────────────────────────────────────

QWidget *MainWindow::buildStatusBar() {
    auto *bar = new QWidget;
    bar->setFixedHeight(36);
    bar->setStyleSheet(QString("background-color: %1;").arg(kBgSurface.name()));

    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(14);

    // CAN status — updated after CAN interface is resolved
    canStatusLabel_ = new QLabel("CAN: …", bar);
    canStatusLabel_->setStyleSheet(
        QString("color: %1; font-size: 11px; font-weight: bold;").arg(kFg3.name()));

    // Current time
    timeLabel_ = new QLabel("00:00", bar);
    timeLabel_->setStyleSheet("color: #ffffff; font-size: 13px; font-weight: bold;");

    // OTA update banner (hidden by default)
    updateBanner_ = new QLabel(bar);
    updateBanner_->setAlignment(Qt::AlignCenter);
    updateBanner_->setStyleSheet(
        "background-color: #0d2408; color: #0fa336;"
        "font-size: 11px; font-weight: bold; padding: 2px 12px;"
        "border: 1px solid #0fa336;");
    updateBanner_->hide();

    // Version
    auto *versionLabel = new QLabel(QString("v%1").arg(APP_VERSION), bar);
    versionLabel->setStyleSheet(
        QString("color: %1; font-size: 11px;").arg(kHairline.name()));

    layout->addWidget(canStatusLabel_);
    layout->addWidget(timeLabel_);
    layout->addWidget(updateBanner_);
    layout->addStretch(1);
    layout->addWidget(versionLabel);

    return bar;
}

// ── Signal wiring ─────────────────────────────────────────────────────────────

void MainWindow::connectSignals() {
    connect(cluster_model_, &ClusterModel::speedChanged,        this, &MainWindow::onSpeedChanged);
    connect(cluster_model_, &ClusterModel::rpmChanged,          this, &MainWindow::onRpmChanged);
    connect(cluster_model_, &ClusterModel::gearChanged,         this, &MainWindow::onGearChanged);
    connect(cluster_model_, &ClusterModel::fuelLevelChanged,    this, &MainWindow::onFuelChanged);
    connect(cluster_model_, &ClusterModel::temperatureChanged,  this, &MainWindow::onTempChanged);
    connect(cluster_model_, &ClusterModel::oilPressureChanged,  this, &MainWindow::onOilPressureChanged);
    connect(cluster_model_, &ClusterModel::switchStatusChanged, this, &MainWindow::onSwitchStatusChanged);
    connect(cluster_model_, &ClusterModel::warningFlagsChanged, this, &MainWindow::onWarningFlagsChanged);
    connect(cluster_model_, &ClusterModel::absActiveChanged,    this, &MainWindow::onAbsActiveChanged);
    connect(cluster_model_, &ClusterModel::tcsActiveChanged,    this, &MainWindow::onTcsActiveChanged);
}

// ── Slots: driving data ───────────────────────────────────────────────────────

void MainWindow::onSpeedChanged(int speed) {
    speedValueLabel_->setText(QString::number(speed));

    // BMW M color thresholds (white → warn amber → M red)
    QString color;
    if (speed < 80)       color = kFg1.name();
    else if (speed < 130) color = kWarning.name();
    else                  color = kMRed.name();

    speedValueLabel_->setStyleSheet(
        QString("color: %1; letter-spacing: -3px;").arg(color));
}

void MainWindow::onRpmChanged(int rpm) {
    if (rpmBar_) rpmBar_->setRpm(rpm);
    if (rpmValueLabel_) {
        // Color turns red above redline
        const bool red = (rpm > 7000);
        rpmValueLabel_->setText(QString::number(rpm));
        rpmValueLabel_->setStyleSheet(
            QString("color: %1; font-size: 13px; font-weight: bold;")
            .arg(red ? kMRed.name() : "#ffffff"));
    }
}

void MainWindow::onGearChanged(int gear) {
    QString letter, desc, colorHex;

    if (gear == 0) {
        letter = "N"; desc = "NEUTRAL";
        colorHex = kFg1.name();                  // white
    } else if (gear == 7) {
        letter = "R"; desc = "REVERSE";
        colorHex = kMRed.name();                 // M red
    } else {
        letter = QString::number(gear);
        desc   = QString("GEAR %1").arg(gear);
        colorHex = kMBlueDark.name();            // M blue
    }

    if (gearLabel_) {
        gearLabel_->setText(letter);
        gearLabel_->setStyleSheet(QString("color: %1;").arg(colorHex));
    }
    if (gearDescLabel_) {
        gearDescLabel_->setText(desc);
    }
}

// ── Slots: engine / fuel ──────────────────────────────────────────────────────

void MainWindow::onFuelChanged(int fuel) {
    if (fuelBar_)    fuelBar_->setPercent(fuel);
    if (fuelPctLabel_) fuelPctLabel_->setText(QString("%1%").arg(fuel));
    if (fuelWarnInd_) fuelWarnInd_->setActive(fuel <= 15);
}

void MainWindow::onTempChanged(int temp) {
    if (!tempLabel_) return;
    const bool hot = (temp >= 110);
    tempLabel_->setText(QString("%1°C").arg(temp));
    tempLabel_->setStyleSheet(
        QString("color: %1; font-size: 26px; font-weight: bold;")
        .arg(hot ? kWarning.name() : kFg1.name()));
}

void MainWindow::onOilPressureChanged(int pressure) {
    if (!oilPressureLabel_) return;
    const bool warn = (pressure < 20);
    oilPressureLabel_->setText(QString::number(pressure));
    oilPressureLabel_->setStyleSheet(
        QString("color: %1; font-size: 26px; font-weight: bold;")
        .arg(warn ? kMRed.name() : kFg1.name()));
    if (oilInd_) oilInd_->setActive(warn);
}

// ── Slots: switch status ──────────────────────────────────────────────────────

void MainWindow::onSwitchStatusChanged(int flags) {
    using namespace SwitchBit;

    const bool turnLeft  = (flags & TurnLeft)  != 0;
    const bool turnRight = (flags & TurnRight) != 0;
    const bool hazard    = (flags & Hazard)    != 0;
    const bool highBeam  = (flags & HighBeam)  != 0;
    const bool ignition  = (flags & Ignition)  != 0;
    const bool engine    = (flags & Engine)    != 0;
    const bool headLight = (flags & HeadLight) != 0;

    // Turn signals / hazard
    if (hazard) {
        turnLeftInd_->startBlink();
        turnRightInd_->startBlink();
    } else {
        if (turnLeft)  turnLeftInd_->startBlink();
        else { turnLeftInd_->stopBlink(); turnLeftInd_->setActive(false); }

        if (turnRight) turnRightInd_->startBlink();
        else { turnRightInd_->stopBlink(); turnRightInd_->setActive(false); }
    }

    highBeamInd_->setActive(highBeam);

    // Status labels — active = white/blue, inactive = #262626
    auto labelStyle = [](bool on, const char *onColor) -> QString {
        return QString("color: %1; font-size: 11px; font-weight: bold; letter-spacing: 1px;")
               .arg(on ? onColor : "#262626");
    };

    ignitionLabel_->setText(ignition ? "IGN: ON"   : "IGN: OFF");
    ignitionLabel_->setStyleSheet(labelStyle(ignition, "#0fa336"));

    engineLabel_->setText(engine ? "ENG: ON"    : "ENG: OFF");
    engineLabel_->setStyleSheet(labelStyle(engine, "#0fa336"));

    headlightLabel_->setText(headLight ? "LIGHT: ON" : "LIGHT: OFF");
    headlightLabel_->setStyleSheet(labelStyle(headLight, "#aaddff"));
}

// ── Slots: warning flags ──────────────────────────────────────────────────────

void MainWindow::onWarningFlagsChanged(int flags) {
    using namespace WarningBit;
    if (checkEngineInd_) checkEngineInd_->setActive((flags & CheckEngine) != 0);
    if (oilInd_ && (flags & OilPressure)) oilInd_->setActive(true);
}

void MainWindow::onAbsActiveChanged(bool active) {
    if (absInd_) absInd_->setActive(active);
}

void MainWindow::onTcsActiveChanged(bool active) {
    if (tcsInd_) tcsInd_->setActive(active);
}

// ── Clock ─────────────────────────────────────────────────────────────────────

void MainWindow::updateClock() {
    if (timeLabel_)
        timeLabel_->setText(QTime::currentTime().toString("HH:mm"));
}

// ── OTA update slots ──────────────────────────────────────────────────────────

void MainWindow::onUpdateAvailable(const QString &version) {
    if (!updateBanner_) return;
    updateBanner_->setText(QString("업데이트 v%1 다운로드 중...").arg(version));
    updateBanner_->show();
    update_manager_->downloadAndApply();
}

void MainWindow::onUpdateProgress(int percent) {
    if (updateBanner_)
        updateBanner_->setText(QString("업데이트 다운로드 %1%...").arg(percent));
}

void MainWindow::onUpdateReady() {
    if (!updateBanner_) return;
    updateBanner_->setStyleSheet(
        "background-color: #0d2408; color: #0fa336;"
        "font-size: 11px; font-weight: bold; padding: 2px 12px;"
        "border: 1px solid #0fa336;");
    updateBanner_->setText("업데이트 완료 — 재시작 중...");
    updateBanner_->show();
    QTimer::singleShot(3000, this, []() {
        QProcess::startDetached("/opt/cluster/cluster", {"--fullscreen"});
        QApplication::quit();
    });
}

void MainWindow::onUpdateError(const QString &msg) {
    if (!updateBanner_) return;
    updateBanner_->setStyleSheet(
        "background-color: #2a0808; color: #e22718;"
        "font-size: 11px; font-weight: bold; padding: 2px 12px;"
        "border: 1px solid #e22718;");
    updateBanner_->setText("업데이트 실패: " + msg);
    updateBanner_->show();
    QTimer::singleShot(5000, updateBanner_, &QLabel::hide);
}

void MainWindow::applyUpdate() {
    update_manager_->downloadAndApply();
}
