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
///   0x400 INFO_SPEED_RPM  → speed
///   0x500 VEHICLE_STATE   → gear
///   0x600 POSITION        → posX, posZ  (cm BE int32 → m double)
///   0x601 HEADING         → heading     (×10 BE uint16 → °  double)
class EntertainmentModel : public QObject {
    Q_OBJECT
public:
    explicit EntertainmentModel(QObject *parent = nullptr);
    ~EntertainmentModel() override = default;

    void setCANInterface(std::unique_ptr<CANInterface> can);
    void startReceiving();

    double posX()    const { return pos_x_; }
    double posZ()    const { return pos_z_; }
    double heading() const { return heading_; }
    int    speed()   const { return speed_; }
    int    gear()    const { return gear_; }

signals:
    void positionChanged(double x, double z);
    void headingChanged(double deg);
    void speedChanged(int speed);
    void gearChanged(int gear);

private slots:
    void onFrameReceived(const can_frame &frame);

private:
    std::unique_ptr<CANInterface> can_;

    double pos_x_{0.0};
    double pos_z_{0.0};
    double heading_{0.0};
    int    speed_{0};
    int    gear_{0};
};

#endif // ENTERTAINMENT_MODEL_HPP
