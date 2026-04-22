#ifndef CLUSTER_MAINWINDOW_HPP
#define CLUSTER_MAINWINDOW_HPP

#include <QMainWindow>
#include <QLabel>
#include <memory>

class ClusterModel;
class UpdateManager;
class GaugeWidget;
class ArcGaugeWidget;
class IndicatorWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // ── 주행 정보 ─────────────────────────────────────────────────────────────
    void onSpeedChanged(int speed);
    void onRpmChanged(int rpm);
    void onGearChanged(int gear);

    // ── 엔진 / 연료 ──────────────────────────────────────────────────────────
    void onFuelChanged(int fuel);
    void onTempChanged(int temp);
    void onOilPressureChanged(int pressure);

    // ── 스위치 / 경고등 ───────────────────────────────────────────────────────
    void onSwitchStatusChanged(int flags);
    void onWarningFlagsChanged(int flags);
    void onAbsActiveChanged(bool active);
    void onTcsActiveChanged(bool active);

    // ── OTA 업데이트 ──────────────────────────────────────────────────────────
    void onUpdateAvailable(const QString &version);
    void onUpdateReady();
    void onUpdateProgress(int percent);
    void onUpdateError(const QString &msg);

private:
    // ── UI 빌더 ───────────────────────────────────────────────────────────────
    void setupUI();
    QWidget *buildIndicatorBar();
    QWidget *buildLeftPanel();
    QWidget *buildCenterPanel();
    QWidget *buildRightPanel();
    QWidget *buildStatusBar();

    void connectSignals();
    void applyUpdate();

    // ── 클러스터 모델 ─────────────────────────────────────────────────────────
    ClusterModel  *cluster_model_{nullptr};
    UpdateManager *update_manager_{nullptr};

    // ── 경고/상태 인디케이터 ───────────────────────────────────────────────────
    IndicatorWidget *turnLeftInd_{nullptr};
    IndicatorWidget *turnRightInd_{nullptr};
    IndicatorWidget *highBeamInd_{nullptr};
    IndicatorWidget *checkEngineInd_{nullptr};
    IndicatorWidget *oilInd_{nullptr};
    IndicatorWidget *absInd_{nullptr};
    IndicatorWidget *tcsInd_{nullptr};
    IndicatorWidget *fuelWarnInd_{nullptr};

    // ── 주요 게이지 ───────────────────────────────────────────────────────────
    GaugeWidget    *rpmGauge_{nullptr};
    ArcGaugeWidget *fuelGauge_{nullptr};
    ArcGaugeWidget *tempGauge_{nullptr};

    // ── 속도 · 기어 표시 ──────────────────────────────────────────────────────
    QLabel *speedValueLabel_{nullptr};    // 큰 속도 숫자
    QLabel *speedUnitLabel_{nullptr};     // "km/h"
    QLabel *gearLabel_{nullptr};          // "N" / "1"~"6" / "R"
    QLabel *gearDescLabel_{nullptr};      // "NEUTRAL" / "GEAR n" / "REVERSE"

    // ── 차량 상태 표시 ────────────────────────────────────────────────────────
    QLabel *ignitionLabel_{nullptr};
    QLabel *engineLabel_{nullptr};
    QLabel *headlightLabel_{nullptr};
    QLabel *oilPressureLabel_{nullptr};

    // ── 상태 바 ───────────────────────────────────────────────────────────────
    QLabel *updateBanner_{nullptr};
    QLabel *canStatusLabel_{nullptr};
};

#endif // CLUSTER_MAINWINDOW_HPP
