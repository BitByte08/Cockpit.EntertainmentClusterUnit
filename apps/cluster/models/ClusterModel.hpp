#ifndef CLUSTER_CLUSTERMODEL_HPP
#define CLUSTER_CLUSTERMODEL_HPP

#include <QObject>
#include <memory>
#include <cstdint>
#include "CANInterface.hpp"

struct can_frame;

// ── SWITCH_STATUS (0x300) 비트 정의 ────────────────────────────────────────
namespace SwitchBit {
    constexpr int Ignition  = 1 << 0;
    constexpr int Engine    = 1 << 1;
    constexpr int HeadLight = 1 << 2;
    constexpr int HighBeam  = 1 << 3;
    constexpr int Hazard    = 1 << 4;
    constexpr int WiperSlow = 1 << 5;
    constexpr int WiperFast = 1 << 6;
    constexpr int Horn      = 1 << 7;
    constexpr int TurnLeft  = 1 << 8;
    constexpr int TurnRight = 1 << 9;
}

// ── INFO_WARNING (0x401) 비트 정의 ──────────────────────────────────────────
namespace WarningBit {
    constexpr int CheckEngine  = 1 << 0;
    constexpr int OilPressure  = 1 << 1;
    constexpr int Battery      = 1 << 2;
    constexpr int BrakeFault   = 1 << 3;
    constexpr int FuelLow      = 1 << 4;
}

class ClusterModel : public QObject {
    Q_OBJECT
public:
    explicit ClusterModel(QObject *parent = nullptr);
    ~ClusterModel() override = default;

    void setCANInterface(std::unique_ptr<CANInterface> can_interface);
    void startReceiving();
    void stopReceiving();

    // ── Getters ──────────────────────────────────────────────────────────────
    int getSpeed() const       { return speed_; }
    int getRpm() const         { return rpm_; }
    int getGear() const        { return gear_; }
    int getFuelLevel() const   { return fuel_level_; }
    int getTemperature() const { return temperature_; }
    int getOilPressure() const { return oil_pressure_; }
    int getSwitchStatus() const  { return switch_status_; }
    int getWarningFlags() const  { return warning_flags_; }
    bool isAbsActive() const   { return abs_active_; }
    bool isTcsActive() const   { return tcs_active_; }

signals:
    // ── 주행 정보 ────────────────────────────────────────────────────────────
    void speedChanged(int speed);        // km/h  (0x400: x10 → ÷10)
    void rpmChanged(int rpm);            // RPM   (0x400)
    void gearChanged(int gear);          // 0=N, 1~6, 7=R  (0x301 / 0x500)

    // ── 스위치 / 경고 ─────────────────────────────────────────────────────────
    void switchStatusChanged(int flags); // SwitchBit 비트필드  (0x300)
    void warningFlagsChanged(int flags); // WarningBit 비트필드 (0x401)
    void absActiveChanged(bool active);  // (0x500)
    void tcsActiveChanged(bool active);  // (0x500)

    // ── 엔진 / 연료 ──────────────────────────────────────────────────────────
    void fuelLevelChanged(int fuel);     // %    (0x501)
    void temperatureChanged(int temp);   // °C   (0x501)
    void oilPressureChanged(int pressure); // 0~100 (0x501)

private slots:
    void onFrameReceived(const can_frame &frame);

private:
    void parseCANFrame(const can_frame &frame);

    std::unique_ptr<CANInterface> can_interface_;

    int speed_{0};
    int rpm_{0};
    int gear_{0};
    int fuel_level_{100};
    int temperature_{90};
    int oil_pressure_{50};
    int switch_status_{0};
    int warning_flags_{0};
    bool abs_active_{false};
    bool tcs_active_{false};
};

#endif // CLUSTER_CLUSTERMODEL_HPP
