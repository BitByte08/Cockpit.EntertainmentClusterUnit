#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "models/ClusterModel.hpp"
#include "widgets/GaugeWidget.hpp"
#include "UpdateManager.hpp"
#ifdef _WIN32
#include "StubCANInterface.hpp"
#else
#include "SocketCANInterface.hpp"
#endif

#include <QVBoxLayout>
#include <QProcess>
#include <memory>

static constexpr const char *kGithubRepo = "BitByte08/Cockpit.EntertainmentClusterUnit";

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      cluster_model_(nullptr),
      update_manager_(nullptr) {

    ui->setupUi(this);

    cluster_model_ = new ClusterModel(this);
#ifdef _WIN32
    auto can_interface = std::make_unique<StubCANInterface>();
#else
    auto can_interface = std::make_unique<SocketCANInterface>("can0");
#endif
    cluster_model_->setCANInterface(std::move(can_interface));

    setupGauges();
    setupInfoPanel();
    setupUpdateBanner();
    connectSignals();

    cluster_model_->startReceiving();

    // 백그라운드에서 업데이트 확인 (앱 시작 5초 후)
    update_manager_ = new UpdateManager(APP_VERSION, kGithubRepo, this);
    connect(update_manager_, &UpdateManager::updateAvailable,
            this, &MainWindow::onUpdateAvailable);
    connect(update_manager_, &UpdateManager::updateProgress,
            this, &MainWindow::onUpdateProgress);
    connect(update_manager_, &UpdateManager::updateReady,
            this, &MainWindow::onUpdateReady);
    connect(update_manager_, &UpdateManager::updateError,
            this, &MainWindow::onUpdateError);

    QTimer::singleShot(5000, update_manager_, &UpdateManager::checkForUpdate);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupGauges() {
    ui->rpmGauge->setMinValue(0);
    ui->rpmGauge->setMaxValue(8000);
    ui->rpmGauge->setMajorTicks(16);
    ui->rpmGauge->setUnit("RPM");
    ui->rpmGauge->setLabel("ENGINE");
}

void MainWindow::setupInfoPanel() {
    ui->infoPanel->setStyleSheet(
        "QFrame#infoPanel {"
        "  background-color: #1C1008;"
        "  border: 2px solid #5C3317;"
        "  border-radius: 12px;"
        "}"
    );

    ui->speedSectionLabel->setStyleSheet(
        "color: #8B6035; font-size: 18px; font-weight: bold;"
        "letter-spacing: 4px; background: transparent;"
    );
    ui->speedValueLabel->setStyleSheet(
        "color: #F0DEB0; font-size: 96px; font-weight: bold;"
        "background: transparent;"
    );
    ui->speedUnitLabel->setStyleSheet(
        "color: #8B6035; font-size: 20px; letter-spacing: 2px;"
        "background: transparent;"
    );

    QString dividerStyle = "color: #3D2210; background-color: #3D2210;";
    ui->divider1->setStyleSheet(dividerStyle);
    ui->divider2->setStyleSheet(dividerStyle);

    ui->fuelSectionLabel->setStyleSheet(
        "color: #7A6048; font-size: 16px; font-weight: bold;"
        "letter-spacing: 3px; background: transparent;"
    );
    ui->fuelValueLabel->setStyleSheet(
        "color: #C8A96E; font-size: 28px; font-weight: bold;"
        "background: transparent;"
    );

    ui->tempSectionLabel->setStyleSheet(
        "color: #7A6048; font-size: 16px; font-weight: bold;"
        "letter-spacing: 3px; background: transparent;"
    );
    ui->tempValueLabel->setStyleSheet(
        "color: #C8A96E; font-size: 28px; font-weight: bold;"
        "background: transparent;"
    );
}

void MainWindow::setupUpdateBanner() {
    // 인포 패널 하단에 업데이트 알림 라벨 추가
    update_banner_ = new QLabel(this);
    update_banner_->setAlignment(Qt::AlignCenter);
    update_banner_->setStyleSheet(
        "background-color: #2A4A1A; color: #80D040;"
        "font-size: 13px; padding: 4px 12px;"
        "border-radius: 6px;"
    );
    update_banner_->hide();

    // infoPanelLayout의 spacer 앞에 배치
    auto *layout = qobject_cast<QVBoxLayout*>(ui->infoPanel->layout());
    if (layout)
        layout->insertWidget(layout->count() - 1, update_banner_);
}

void MainWindow::connectSignals() {
    connect(cluster_model_, &ClusterModel::rpmChanged,
            ui->rpmGauge, &GaugeWidget::setValue);

    connect(cluster_model_, &ClusterModel::speedChanged,
            this, &MainWindow::onSpeedChanged);
    connect(cluster_model_, &ClusterModel::fuelLevelChanged,
            this, &MainWindow::onFuelChanged);
    connect(cluster_model_, &ClusterModel::temperatureChanged,
            this, &MainWindow::onTempChanged);
}

// ── 데이터 업데이트 슬롯 ─────────────────────────────────────────────────────

void MainWindow::onSpeedChanged(int speed) {
    ui->speedValueLabel->setText(QString::number(speed));

    QString color;
    if (speed < 80)        color = "#F0DEB0";
    else if (speed < 130)  color = "#D4A030";
    else                   color = "#C03020";

    ui->speedValueLabel->setStyleSheet(
        QString("color: %1; font-size: 96px; font-weight: bold; background: transparent;")
            .arg(color)
    );
}

void MainWindow::onFuelChanged(int fuel) {
    QString color = (fuel <= 15) ? "#C03020" : "#C8A96E";
    ui->fuelValueLabel->setText(QString::number(fuel) + "%");
    ui->fuelValueLabel->setStyleSheet(
        QString("color: %1; font-size: 28px; font-weight: bold; background: transparent;")
            .arg(color)
    );
}

void MainWindow::onTempChanged(int temp) {
    QString color = (temp >= 110) ? "#C03020" : "#C8A96E";
    ui->tempValueLabel->setText(QString::number(temp) + "°C");
    ui->tempValueLabel->setStyleSheet(
        QString("color: %1; font-size: 28px; font-weight: bold; background: transparent;")
            .arg(color)
    );
}

// ── OTA 업데이트 슬롯 ────────────────────────────────────────────────────────

void MainWindow::onUpdateAvailable(const QString &version) {
    if (!update_banner_) return;
    update_banner_->setText(
        QString("업데이트 v%1 다운로드 중...").arg(version)
    );
    update_banner_->show();
    // 자동으로 다운로드 시작
    update_manager_->downloadAndApply();
}

void MainWindow::onUpdateProgress(int percent) {
    if (!update_banner_) return;
    update_banner_->setText(
        QString("업데이트 다운로드 %1%...").arg(percent)
    );
}

void MainWindow::onUpdateReady() {
    if (!update_banner_) return;
    update_banner_->setStyleSheet(
        "background-color: #1A3A10; color: #60C030;"
        "font-size: 13px; padding: 4px 12px; border-radius: 6px;"
    );
    update_banner_->setText("업데이트 완료 — 재시작 중...");
    update_banner_->show();

    // 3초 후 앱 재시작 (systemd Restart=always가 새 바이너리 기동)
    QTimer::singleShot(3000, this, []() {
        QProcess::startDetached("/opt/cluster/cluster", {"--fullscreen"});
        QApplication::quit();
    });
}

void MainWindow::onUpdateError(const QString &msg) {
    if (!update_banner_) return;
    update_banner_->setStyleSheet(
        "background-color: #3A1010; color: #D05030;"
        "font-size: 13px; padding: 4px 12px; border-radius: 6px;"
    );
    update_banner_->setText("업데이트 실패: " + msg);
    update_banner_->show();

    // 5초 후 배너 숨김
    QTimer::singleShot(5000, update_banner_, &QLabel::hide);
}
