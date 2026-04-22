#include "mainwindow.hpp"
#include "models/ClusterModel.hpp"
#include "widgets/GaugeWidget.hpp"
#include "widgets/ArcGaugeWidget.hpp"
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
#include <QFont>
#include <QApplication>

static constexpr const char *kGithubRepo = "BitByte08/Cockpit.EntertainmentClusterUnit";

// ── 색상 상수 ─────────────────────────────────────────────────────────────────
static const QColor kBgDeep   {0x05, 0x05, 0x08};
static const QColor kBgPanel  {0x0A, 0x0A, 0x10};
static const QColor kBgAlt    {0x07, 0x07, 0x0F};
static const QColor kDivider  {0x18, 0x18, 0x28};
static const QColor kAccent   {0x00, 0xCC, 0xBB};
static const QColor kTextDim  {0x66, 0x66, 0x88};

// ── 헬퍼: 구분선 위젯 ────────────────────────────────────────────────────────

static QFrame *makeHLine(QWidget *parent) {
    auto *f = new QFrame(parent);
    f->setFrameShape(QFrame::HLine);
    f->setFixedHeight(1);
    f->setStyleSheet("background-color: #181828; border: none;");
    return f;
}

static QFrame *makeVLine(QWidget *parent) {
    auto *f = new QFrame(parent);
    f->setFrameShape(QFrame::VLine);
    f->setFixedWidth(1);
    f->setStyleSheet("background-color: #181828; border: none;");
    return f;
}

// ── 생성자 ────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Digital Cluster");
    setMinimumSize(1024, 600);
    setStyleSheet(QString("QMainWindow, QWidget { background-color: %1; }")
                  .arg(kBgDeep.name()));

    // ── 클러스터 모델 초기화 ────────────────────────────────────────────────
    cluster_model_ = new ClusterModel(this);

    setupUI();
    connectSignals();

#ifdef _WIN32
    cluster_model_->setCANInterface(std::make_unique<StubCANInterface>());
    canStatusLabel_->setText("CAN: stub  ○");
    canStatusLabel_->setStyleSheet("color: #886600; font-size: 11px;");
#else
    // can0 없으면 stub으로 fallback (개발 환경)
    bool canOk = false;
    try {
        cluster_model_->setCANInterface(std::make_unique<SocketCANInterface>("can0"));
        canOk = true;
    } catch (const std::exception &) {
        cluster_model_->setCANInterface(std::make_unique<StubCANInterface>());
    }
    if (canOk) {
        canStatusLabel_->setText("CAN: can0  ●");
        canStatusLabel_->setStyleSheet("color: #00CC44; font-size: 11px;");
    } else {
        canStatusLabel_->setText("CAN: stub  ○");
        canStatusLabel_->setStyleSheet("color: #886600; font-size: 11px;");
    }
#endif

    cluster_model_->startReceiving();

    // ── OTA 업데이트 매니저 ─────────────────────────────────────────────────
    update_manager_ = new UpdateManager(APP_VERSION, kGithubRepo, this);
    connect(update_manager_, &UpdateManager::updateAvailable, this, &MainWindow::onUpdateAvailable);
    connect(update_manager_, &UpdateManager::updateProgress,  this, &MainWindow::onUpdateProgress);
    connect(update_manager_, &UpdateManager::updateReady,     this, &MainWindow::onUpdateReady);
    connect(update_manager_, &UpdateManager::updateError,     this, &MainWindow::onUpdateError);
    QTimer::singleShot(5000, update_manager_, &UpdateManager::checkForUpdate);
}

// ── UI 구성 ───────────────────────────────────────────────────────────────────

void MainWindow::setupUI() {
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *root = new QVBoxLayout(central);
    root->setSpacing(0);
    root->setContentsMargins(0, 0, 0, 0);

    root->addWidget(buildIndicatorBar());
    root->addWidget(makeHLine(central));

    // 메인 영역: 좌(RPM) | 중앙(속도·기어) | 우(연료·수온)
    auto *mainArea = new QWidget(central);
    mainArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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

// ── 경고등 바 (상단) ─────────────────────────────────────────────────────────

QWidget *MainWindow::buildIndicatorBar() {
    auto *bar = new QWidget;
    bar->setFixedHeight(58);
    bar->setStyleSheet(QString("background-color: %1;").arg(kBgPanel.name()));

    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(20, 8, 20, 8);
    layout->setSpacing(10);

    // 위젯 생성
    turnLeftInd_    = new IndicatorWidget(IndicatorIcon::TurnLeft,    "TURN L",  QColor(0x00, 0xCC, 0x44), bar);
    highBeamInd_    = new IndicatorWidget(IndicatorIcon::HighBeam,    "HI BEAM", QColor(0x00, 0xAA, 0xFF), bar);
    checkEngineInd_ = new IndicatorWidget(IndicatorIcon::CheckEngine, "ENGINE",  QColor(0xFF, 0x8C, 0x00), bar);
    oilInd_         = new IndicatorWidget(IndicatorIcon::OilPressure, "OIL",     QColor(0xFF, 0x33, 0x33), bar);
    absInd_         = new IndicatorWidget(IndicatorIcon::ABS,         "ABS",     QColor(0xFF, 0x8C, 0x00), bar);
    tcsInd_         = new IndicatorWidget(IndicatorIcon::TCS,         "TCS",     QColor(0xFF, 0x8C, 0x00), bar);
    fuelWarnInd_    = new IndicatorWidget(IndicatorIcon::FuelWarn,    "FUEL",    QColor(0xFF, 0xB3, 0x00), bar);
    turnRightInd_   = new IndicatorWidget(IndicatorIcon::TurnRight,   "TURN R",  QColor(0x00, 0xCC, 0x44), bar);

    for (auto *w : {(QWidget*)turnLeftInd_,  (QWidget*)highBeamInd_,
                    (QWidget*)checkEngineInd_,(QWidget*)oilInd_,
                    (QWidget*)absInd_,        (QWidget*)tcsInd_,
                    (QWidget*)fuelWarnInd_,   (QWidget*)turnRightInd_}) {
        w->setFixedSize(54, 42);
    }

    // 레이아웃: [←] [BEAM] ─ spacer ─ [ENG] [OIL] [ABS] [TCS] ─ spacer ─ [FUEL] [→]
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

// ── 좌측 패널 (RPM 게이지) ────────────────────────────────────────────────────

QWidget *MainWindow::buildLeftPanel() {
    auto *panel = new QWidget;
    panel->setFixedWidth(295);
    panel->setStyleSheet(QString("background-color: %1;").arg(kBgPanel.name()));

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 12, 10, 12);
    layout->setSpacing(6);

    // RPM 게이지
    rpmGauge_ = new GaugeWidget(panel);
    rpmGauge_->setMinValue(0);
    rpmGauge_->setMaxValue(8000);
    rpmGauge_->setMajorTicks(8);
    rpmGauge_->setRedZoneStart(6500);
    rpmGauge_->setUnit("RPM");
    rpmGauge_->setLabel("ENGINE RPM");
    rpmGauge_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(rpmGauge_, 1);

    // 기어 미니 표시 (RPM 아래)
    auto *gearRow = new QWidget(panel);
    gearRow->setFixedHeight(38);
    gearRow->setStyleSheet("background-color: transparent;");
    auto *gearRowLayout = new QHBoxLayout(gearRow);
    gearRowLayout->setContentsMargins(6, 0, 6, 0);
    gearRowLayout->setSpacing(6);

    auto *gearTag = new QLabel("GEAR", gearRow);
    gearTag->setStyleSheet("color: #446688; font-size: 10px; font-weight: bold; letter-spacing: 2px;");

    gearLabel_ = new QLabel("N", gearRow);
    gearLabel_->setStyleSheet("color: #CCCCDD; font-size: 24px; font-weight: bold;");
    gearLabel_->setAlignment(Qt::AlignCenter);

    gearDescLabel_ = new QLabel("NEUTRAL", gearRow);
    gearDescLabel_->setStyleSheet("color: #666688; font-size: 9px; letter-spacing: 1px;");

    gearRowLayout->addWidget(gearTag);
    gearRowLayout->addWidget(gearLabel_);
    gearRowLayout->addStretch(1);
    gearRowLayout->addWidget(gearDescLabel_);

    layout->addWidget(gearRow);
    return panel;
}

// ── 중앙 패널 (속도 · 차량 상태) ─────────────────────────────────────────────

QWidget *MainWindow::buildCenterPanel() {
    auto *panel = new QWidget;
    panel->setStyleSheet(QString("background-color: %1;").arg(kBgAlt.name()));

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(0);

    // ── 속도 (상단 flex) ──────────────────────────────────────────────────────
    layout->addStretch(2);

    speedValueLabel_ = new QLabel("0", panel);
    {
        QFont f;
        f.setPointSize(80);
        f.setBold(true);
        f.setStyleHint(QFont::SansSerif);
        speedValueLabel_->setFont(f);
    }
    speedValueLabel_->setAlignment(Qt::AlignCenter);
    speedValueLabel_->setStyleSheet("color: #FFFFFF; background: transparent;");
    layout->addWidget(speedValueLabel_);

    speedUnitLabel_ = new QLabel("km/h", panel);
    speedUnitLabel_->setAlignment(Qt::AlignCenter);
    speedUnitLabel_->setStyleSheet("color: #8888AA; font-size: 18px; letter-spacing: 4px; background: transparent;");
    layout->addWidget(speedUnitLabel_);

    layout->addStretch(1);

    // ── 구분선 ────────────────────────────────────────────────────────────────
    auto *sep = new QFrame(panel);
    sep->setFrameShape(QFrame::HLine);
    sep->setFixedHeight(1);
    sep->setStyleSheet("background-color: #1A1A30; border: none;");
    layout->addWidget(sep);

    layout->addStretch(1);

    // ── 기어 표시 (하단) ──────────────────────────────────────────────────────
    auto *gearBig = new QLabel("N", panel);
    {
        QFont f;
        f.setPointSize(52);
        f.setBold(true);
        gearBig->setFont(f);
    }
    gearBig->setAlignment(Qt::AlignCenter);
    gearBig->setStyleSheet("color: #CCCCDD; background: transparent;");
    gearBig->setObjectName("centerGearLabel");
    layout->addWidget(gearBig);

    auto *gearDescCenter = new QLabel("NEUTRAL", panel);
    gearDescCenter->setAlignment(Qt::AlignCenter);
    gearDescCenter->setStyleSheet("color: #445566; font-size: 11px; letter-spacing: 3px; background: transparent;");
    gearDescCenter->setObjectName("centerGearDesc");
    layout->addWidget(gearDescCenter);

    layout->addStretch(1);

    // ── 차량 상태 행 ──────────────────────────────────────────────────────────
    auto *statusRow = new QWidget(panel);
    statusRow->setFixedHeight(30);
    statusRow->setStyleSheet("background: transparent;");
    auto *statusLayout = new QHBoxLayout(statusRow);
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(16);

    ignitionLabel_ = new QLabel("IGN: OFF", statusRow);
    ignitionLabel_->setStyleSheet("color: #404055; font-size: 11px; font-weight: bold;");

    engineLabel_ = new QLabel("ENG: OFF", statusRow);
    engineLabel_->setStyleSheet("color: #404055; font-size: 11px; font-weight: bold;");

    headlightLabel_ = new QLabel("LIGHT: OFF", statusRow);
    headlightLabel_->setStyleSheet("color: #404055; font-size: 11px; font-weight: bold;");

    statusLayout->addStretch(1);
    statusLayout->addWidget(ignitionLabel_);
    statusLayout->addWidget(engineLabel_);
    statusLayout->addWidget(headlightLabel_);
    statusLayout->addStretch(1);

    layout->addWidget(statusRow);
    layout->addStretch(1);

    return panel;
}

// ── 우측 패널 (연료 · 수온) ───────────────────────────────────────────────────

QWidget *MainWindow::buildRightPanel() {
    auto *panel = new QWidget;
    panel->setFixedWidth(270);
    panel->setStyleSheet(QString("background-color: %1;").arg(kBgPanel.name()));

    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(10, 12, 10, 12);
    layout->setSpacing(0);

    // 연료 게이지
    fuelGauge_ = new ArcGaugeWidget(panel);
    fuelGauge_->setRange(0, 100);
    fuelGauge_->setLabel("FUEL");
    fuelGauge_->setUnit("%");
    fuelGauge_->setWarnThreshold(15, false);
    fuelGauge_->setValue(cluster_model_->getFuelLevel());
    fuelGauge_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(fuelGauge_, 1);

    layout->addWidget(makeHLine(panel));

    // 수온 게이지
    tempGauge_ = new ArcGaugeWidget(panel);
    tempGauge_->setRange(60, 130);
    tempGauge_->setLabel("TEMP");
    tempGauge_->setUnit("°C");
    tempGauge_->setWarnThreshold(110, true);
    tempGauge_->setValue(cluster_model_->getTemperature());
    tempGauge_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(tempGauge_, 1);

    // 유압 행
    auto *oilRow = new QWidget(panel);
    oilRow->setFixedHeight(28);
    oilRow->setStyleSheet("background: transparent;");
    auto *oilLayout = new QHBoxLayout(oilRow);
    oilLayout->setContentsMargins(4, 0, 4, 0);

    auto *oilTag = new QLabel("OIL PRESS", oilRow);
    oilTag->setStyleSheet("color: #446688; font-size: 9px; font-weight: bold; letter-spacing: 1px;");

    oilPressureLabel_ = new QLabel("50", oilRow);
    oilPressureLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    oilPressureLabel_->setStyleSheet("color: #CCCCDD; font-size: 14px; font-weight: bold;");

    oilLayout->addWidget(oilTag);
    oilLayout->addStretch(1);
    oilLayout->addWidget(oilPressureLabel_);

    layout->addWidget(oilRow);
    return panel;
}

// ── 상태 바 (하단) ────────────────────────────────────────────────────────────

QWidget *MainWindow::buildStatusBar() {
    auto *bar = new QWidget;
    bar->setFixedHeight(36);
    bar->setStyleSheet(QString("background-color: %1;").arg(kBgPanel.name()));

    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(16, 0, 16, 0);
    layout->setSpacing(12);

    canStatusLabel_ = new QLabel("CAN: can0  ●", bar);
    canStatusLabel_->setStyleSheet("color: #00CC44; font-size: 11px;");

    updateBanner_ = new QLabel(bar);
    updateBanner_->setAlignment(Qt::AlignCenter);
    updateBanner_->setStyleSheet(
        "background-color: #1A3A10; color: #00FF66;"
        "font-size: 11px; padding: 2px 12px;"
        "border-radius: 3px; border: 1px solid #00CC44;"
    );
    updateBanner_->hide();

    auto *versionLabel = new QLabel(QString("v%1").arg(APP_VERSION), bar);
    versionLabel->setStyleSheet("color: #444466; font-size: 11px;");

    layout->addWidget(canStatusLabel_);
    layout->addWidget(updateBanner_);
    layout->addStretch(1);
    layout->addWidget(versionLabel);

    return bar;
}

// ── 시그널 연결 ───────────────────────────────────────────────────────────────

void MainWindow::connectSignals() {
    connect(cluster_model_, &ClusterModel::rpmChanged,          rpmGauge_, &GaugeWidget::setValue);
    connect(cluster_model_, &ClusterModel::fuelLevelChanged,    this,      &MainWindow::onFuelChanged);
    connect(cluster_model_, &ClusterModel::temperatureChanged,  this,      &MainWindow::onTempChanged);
    connect(cluster_model_, &ClusterModel::oilPressureChanged,  this,      &MainWindow::onOilPressureChanged);
    connect(cluster_model_, &ClusterModel::speedChanged,        this,      &MainWindow::onSpeedChanged);
    connect(cluster_model_, &ClusterModel::rpmChanged,          this,      &MainWindow::onRpmChanged);
    connect(cluster_model_, &ClusterModel::gearChanged,         this,      &MainWindow::onGearChanged);
    connect(cluster_model_, &ClusterModel::switchStatusChanged, this,      &MainWindow::onSwitchStatusChanged);
    connect(cluster_model_, &ClusterModel::warningFlagsChanged, this,      &MainWindow::onWarningFlagsChanged);
    connect(cluster_model_, &ClusterModel::absActiveChanged,    this,      &MainWindow::onAbsActiveChanged);
    connect(cluster_model_, &ClusterModel::tcsActiveChanged,    this,      &MainWindow::onTcsActiveChanged);
}

// ── 슬롯: 주행 정보 ───────────────────────────────────────────────────────────

void MainWindow::onSpeedChanged(int speed) {
    speedValueLabel_->setText(QString::number(speed));

    QString color;
    if (speed < 80)        color = "#FFFFFF";
    else if (speed < 130)  color = "#FFB300";
    else                   color = "#FF3333";

    speedValueLabel_->setStyleSheet(
        QString("color: %1; background: transparent;").arg(color));
}

void MainWindow::onRpmChanged(int /*rpm*/) {
    // rpmGauge_는 직접 연결됨; 여기서는 추가 처리 불필요
}

void MainWindow::onGearChanged(int gear) {
    QString letter, desc, colorStyle;

    if (gear == 0) {
        letter = "N";  desc = "NEUTRAL";
        colorStyle = "color: #CCCCDD;";
    } else if (gear == 7) {
        letter = "R";  desc = "REVERSE";
        colorStyle = "color: #FF4444;";
    } else {
        letter = QString::number(gear);
        desc   = QString("GEAR %1").arg(gear);
        colorStyle = "color: #00CCBB;";
    }

    // 좌측 패널 기어 라벨
    if (gearLabel_)     gearLabel_->setText(letter);
    if (gearDescLabel_) gearDescLabel_->setText(desc);
    gearLabel_->setStyleSheet(colorStyle + " font-size: 24px; font-weight: bold;");

    // 중앙 패널 큰 기어 라벨
    if (auto *w = findChild<QLabel*>("centerGearLabel")) {
        w->setText(letter);
        w->setStyleSheet(colorStyle + " background: transparent;");
    }
    if (auto *w = findChild<QLabel*>("centerGearDesc")) {
        w->setText(desc);
    }
}

// ── 슬롯: 엔진/연료 ──────────────────────────────────────────────────────────

void MainWindow::onFuelChanged(int fuel) {
    fuelGauge_->setValue(fuel);
    fuelWarnInd_->setActive(fuel <= 15);
}

void MainWindow::onTempChanged(int temp) {
    tempGauge_->setValue(temp);
}

void MainWindow::onOilPressureChanged(int pressure) {
    bool warn = (pressure < 20);
    oilPressureLabel_->setText(QString::number(pressure));
    oilPressureLabel_->setStyleSheet(
        QString("color: %1; font-size: 14px; font-weight: bold;")
            .arg(warn ? "#FF3333" : "#CCCCDD"));
    oilInd_->setActive(warn);
}

// ── 슬롯: 스위치 상태 ────────────────────────────────────────────────────────

void MainWindow::onSwitchStatusChanged(int flags) {
    using namespace SwitchBit;

    bool turnLeft  = (flags & TurnLeft)  != 0;
    bool turnRight = (flags & TurnRight) != 0;
    bool hazard    = (flags & Hazard)    != 0;
    bool highBeam  = (flags & HighBeam)  != 0;
    bool ignition  = (flags & Ignition)  != 0;
    bool engine    = (flags & Engine)    != 0;
    bool headLight = (flags & HeadLight) != 0;

    // ── 방향지시등 / 비상등 ────────────────────────────────────────────────
    if (hazard) {
        turnLeftInd_->startBlink();
        turnRightInd_->startBlink();
    } else {
        if (turnLeft) turnLeftInd_->startBlink();
        else          { turnLeftInd_->stopBlink(); turnLeftInd_->setActive(false); }

        if (turnRight) turnRightInd_->startBlink();
        else           { turnRightInd_->stopBlink(); turnRightInd_->setActive(false); }
    }

    // ── 상향등 ────────────────────────────────────────────────────────────
    highBeamInd_->setActive(highBeam);

    // ── 이그니션 / 시동 / 헤드라이트 상태 표시 ──────────────────────────────
    auto makeStatusStyle = [](bool on, const QString &onColor) -> QString {
        return QString("color: %1; font-size: 11px; font-weight: bold;")
               .arg(on ? onColor : "#404055");
    };

    ignitionLabel_->setText(ignition ? "IGN: ON" : "IGN: OFF");
    ignitionLabel_->setStyleSheet(makeStatusStyle(ignition, "#00CC44"));

    engineLabel_->setText(engine ? "ENG: ON" : "ENG: OFF");
    engineLabel_->setStyleSheet(makeStatusStyle(engine, "#00CC44"));

    headlightLabel_->setText(headLight ? "LIGHT: ON" : "LIGHT: OFF");
    headlightLabel_->setStyleSheet(makeStatusStyle(headLight, "#AADDFF"));
}

// ── 슬롯: 경고 플래그 ────────────────────────────────────────────────────────

void MainWindow::onWarningFlagsChanged(int flags) {
    using namespace WarningBit;
    checkEngineInd_->setActive((flags & CheckEngine) != 0);
    // OilPressure 경고등은 onOilPressureChanged에서도 처리하지만 비트로도 반영
    if (flags & OilPressure) oilInd_->setActive(true);
}

void MainWindow::onAbsActiveChanged(bool active) {
    absInd_->setActive(active);
}

void MainWindow::onTcsActiveChanged(bool active) {
    tcsInd_->setActive(active);
}

// ── OTA 업데이트 슬롯 ────────────────────────────────────────────────────────

void MainWindow::onUpdateAvailable(const QString &version) {
    if (!updateBanner_) return;
    updateBanner_->setText(QString("업데이트 v%1 다운로드 중...").arg(version));
    updateBanner_->show();
    update_manager_->downloadAndApply();
}

void MainWindow::onUpdateProgress(int percent) {
    if (!updateBanner_) return;
    updateBanner_->setText(QString("업데이트 다운로드 %1%...").arg(percent));
}

void MainWindow::onUpdateReady() {
    if (!updateBanner_) return;
    updateBanner_->setStyleSheet(
        "background-color: #1A3A10; color: #60C030;"
        "font-size: 11px; padding: 2px 12px; border-radius: 3px;"
    );
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
        "background-color: #3A1010; color: #FF4444;"
        "font-size: 11px; padding: 2px 12px; border-radius: 3px;"
    );
    updateBanner_->setText("업데이트 실패: " + msg);
    updateBanner_->show();
    QTimer::singleShot(5000, updateBanner_, &QLabel::hide);
}

void MainWindow::applyUpdate() {
    update_manager_->downloadAndApply();
}
