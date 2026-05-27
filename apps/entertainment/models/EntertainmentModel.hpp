#ifndef ENTERTAINMENT_MODEL_HPP
#define ENTERTAINMENT_MODEL_HPP

#include <QObject>
#include <memory>
#include <cstdint>
#include "CANInterface.hpp"

struct can_frame;

/// CAN 수신 → 엔터테인먼트 데이터 파싱
///
/// 수신 ID:
///   0x400 INFO_SPEED_RPM  → speed, rpm
///   0x500 VEHICLE_STATE   → gear
///   0x501 ENGINE_STATE    → coolant(°C), oil(%), fuel(%)
///   0x600 POSITION        → posX, posZ  (cm BE int32 → m double)
///   0x601 HEADING         → heading     (×10 BE uint16 → °  double)
class EntertainmentModel : public QObject {
    Q_OBJECT
public:
    explicit EntertainmentModel(QObject *parent = nullptr);
    ~EntertainmentModel() override = default;

    void setCANInterface(std::unique_ptr<CANInterface> can);
    void startReceiving();

    double   posX()        const { return pos_x_; }
    double   posZ()        const { return pos_z_; }
    double   heading()     const { return heading_; }
    int      speed()       const { return speed_; }
    int      rpm()         const { return rpm_; }
    int      gear()        const { return gear_; }
    int      coolant()     const { return coolant_; }
    int      oilPct()      const { return oil_pct_; }
    int      fuelPct()     const { return fuel_pct_; }
    // 0x502 Driving Dynamics
    int      transmittedTorque() const { return torque_; }
    double   lateralG()          const { return lat_g_; }
    double   longitudinalG()     const { return lon_g_; }
    // 0x503 ADAS Status
    bool     absActive()         const { return abs_active_; }
    bool     tcsActive()         const { return tcs_active_; }
    uint8_t  wheelLockBits()     const { return wheel_lock_; } // bit0=FL,1=FR,2=RL,3=RR

    uint16_t switchFlags() const { return sw_flags_; }
    uint16_t turnFlags()   const { return turn_flags_; }

    bool engineRunning()   const { return rpm_ > 200; }

    // 0x300 스위치 패널 프레임 전송 (메인 스레드에서 호출 안전)
    void sendSwitchFlags(uint16_t flags);

    // 0x700 내비게이션 방향 → 클러스터 브로드캐스트
    // type: 0=None,1=Straight,2=TurnLeft,3=TurnRight,4=Arrived
    void sendManeuver(int type, int distMeters);

signals:
    void positionChanged(double x, double z);
    void headingChanged(double deg);
    void speedChanged(int kmh);
    void rpmChanged(int rpm);
    void gearChanged(int gear);
    void engineStateChanged(int coolant, int oilPct, int fuelPct);
    void drivingDynamicsChanged(int torque, double latG, double lonG);
    void adasStatusChanged(bool absActive, bool tcsActive, uint8_t wheelLock);
    void switchFlagsChanged(uint16_t flags);   // 0x300 수신/송신 시
    void turnFlagsChanged(uint16_t flags);     // 0x101 수신 시

private slots:
    void onFrameReceived(const can_frame &frame);

private:
    std::unique_ptr<CANInterface> can_;

    double   pos_x_{0.0};
    double   pos_z_{0.0};
    double   heading_{0.0};
    int      speed_{0};
    int      rpm_{0};
    int      gear_{0};
    int      coolant_{0};
    int      oil_pct_{0};
    int      fuel_pct_{0};
    int      torque_{0};
    double   lat_g_{0.0};
    double   lon_g_{0.0};
    bool     abs_active_{false};
    bool     tcs_active_{false};
    uint8_t  wheel_lock_{0};
    uint16_t sw_flags_{0};
    uint16_t turn_flags_{0};
};

#endif // ENTERTAINMENT_MODEL_HPP
