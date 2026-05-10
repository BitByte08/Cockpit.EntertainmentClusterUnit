#ifndef CLUSTER_MAINWINDOW_HPP
#define CLUSTER_MAINWINDOW_HPP

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <memory>

class ClusterModel;
class UpdateManager;
class RpmBarWidget;
class FuelBarWidget;
class IndicatorWidget;

// ── MainWindow ────────────────────────────────────────────────────────────────
// BMW M "Center Speed" cluster layout (Layout B from design spec).
// 800 × 480 target; scales on larger displays.
//
// Layout stack (top → bottom):
//   Indicator bar  44 px   — warning lights (turn, beam, engine, ABS, TCS…)
//   M stripe        3 px   — tricolor #0066b1 | #1c69d4 | #e22718
//   RPM bar row    44 px   — discrete-cell bar + RPM value
//   ─ hairline ─
//   Main area      flex    — Left (gear+fuel) | Center (speed) | Right (metrics)
//   ─ hairline ─
//   Status bar     36 px   — clock + CAN status + update banner + version
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // ── Driving data ─────────────────────────────────────────────────────────
    void onSpeedChanged(int speed);
    void onRpmChanged(int rpm);
    void onGearChanged(int gear);

    // ── Engine / fuel ────────────────────────────────────────────────────────
    void onFuelChanged(int fuel);
    void onTempChanged(int temp);
    void onOilPressureChanged(int pressure);

    // ── Switch / warning ─────────────────────────────────────────────────────
    void onSwitchStatusChanged(int flags);
    void onWarningFlagsChanged(int flags);
    void onAbsActiveChanged(bool active);
    void onTcsActiveChanged(bool active);

    // ── OTA update ───────────────────────────────────────────────────────────
    void onUpdateAvailable(const QString &version);
    void onUpdateReady();
    void onUpdateProgress(int percent);
    void onUpdateError(const QString &msg);

    // ── Clock ────────────────────────────────────────────────────────────────
    void updateClock();

private:
    void setupUI();
    QWidget *buildIndicatorBar();
    QWidget *buildRpmBarRow();
    QWidget *buildLeftPanel();
    QWidget *buildCenterPanel();
    QWidget *buildRightPanel();
    QWidget *buildStatusBar();

    void connectSignals();
    void applyUpdate();

    // ── Model ────────────────────────────────────────────────────────────────
    ClusterModel  *cluster_model_{nullptr};
    UpdateManager *update_manager_{nullptr};

    // ── Indicator lights ─────────────────────────────────────────────────────
    IndicatorWidget *turnLeftInd_{nullptr};
    IndicatorWidget *turnRightInd_{nullptr};
    IndicatorWidget *highBeamInd_{nullptr};
    IndicatorWidget *checkEngineInd_{nullptr};
    IndicatorWidget *oilInd_{nullptr};
    IndicatorWidget *absInd_{nullptr};
    IndicatorWidget *tcsInd_{nullptr};
    IndicatorWidget *fuelWarnInd_{nullptr};

    // ── RPM bar row ──────────────────────────────────────────────────────────
    RpmBarWidget *rpmBar_{nullptr};
    QLabel       *rpmValueLabel_{nullptr};

    // ── Left panel: gear + fuel ───────────────────────────────────────────────
    QLabel        *gearLabel_{nullptr};
    QLabel        *gearDescLabel_{nullptr};
    FuelBarWidget *fuelBar_{nullptr};
    QLabel        *fuelPctLabel_{nullptr};

    // ── Center panel: speed ──────────────────────────────────────────────────
    QLabel *speedValueLabel_{nullptr};

    // ── Right panel: status + metrics ─────────────────────────────────────────
    QLabel *ignitionLabel_{nullptr};
    QLabel *engineLabel_{nullptr};
    QLabel *headlightLabel_{nullptr};
    QLabel *tempLabel_{nullptr};
    QLabel *oilPressureLabel_{nullptr};

    // ── Status bar ───────────────────────────────────────────────────────────
    QLabel *canStatusLabel_{nullptr};
    QLabel *updateBanner_{nullptr};
    QLabel *timeLabel_{nullptr};
    QTimer *clockTimer_{nullptr};
};

#endif // CLUSTER_MAINWINDOW_HPP
