#include "ClusterModel.hpp"
#ifndef _WIN32
#include <linux/can.h>
#endif
#include <QThread>

ClusterModel::ClusterModel(QObject *parent) : QObject(parent) {
    qRegisterMetaType<can_frame>("can_frame");
}

void ClusterModel::setCANInterface(std::unique_ptr<CANInterface> can_interface) {
    can_interface_ = std::move(can_interface);
    connect(can_interface_.get(), &CANInterface::frameReceived,
            this, &ClusterModel::onFrameReceived);
}

void ClusterModel::startReceiving() {
    if (!can_interface_) return;
    QThread *thread = new QThread;
    can_interface_->moveToThread(thread);
    connect(thread, &QThread::started, can_interface_.get(), &CANInterface::start);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void ClusterModel::stopReceiving() {
    if (can_interface_) can_interface_->stop();
}

void ClusterModel::onFrameReceived(const can_frame &frame) {
    parseCANFrame(frame);
}

void ClusterModel::parseCANFrame(const can_frame &frame) {
    switch (frame.can_id) {

    // ── 0x300 SWITCH_STATUS: uint16 비트필드 (little-endian) ────────────────
    case 0x300: {
        if (frame.can_dlc < 2) break;
        int flags = static_cast<int>(frame.data[0])
                  | (static_cast<int>(frame.data[1]) << 8);
        if (flags != switch_status_) {
            switch_status_ = flags;
            emit switchStatusChanged(switch_status_);
        }
        break;
    }

    // ── 0x301 GEAR_STATUS: byte (0=N, 1~6, 7=R) ────────────────────────────
    case 0x301: {
        if (frame.can_dlc < 1) break;
        int gear = static_cast<int>(frame.data[0]);
        if (gear != gear_) {
            gear_ = gear;
            emit gearChanged(gear_);
        }
        break;
    }

    // ── 0x400 INFO_SPEED_RPM: [속도u16×10 BE][RPM u16 BE] ─────────────────
    case 0x400: {
        if (frame.can_dlc < 4) break;
        int speed = ((static_cast<int>(frame.data[0]) << 8) | frame.data[1]) / 10;
        int rpm   = (static_cast<int>(frame.data[2]) << 8) | frame.data[3];
        if (speed != speed_) { speed_ = speed; emit speedChanged(speed_); }
        if (rpm   != rpm_)   { rpm_   = rpm;   emit rpmChanged(rpm_); }
        break;
    }

    // ── 0x401 INFO_WARNING: uint16 비트필드 (little-endian) ─────────────────
    case 0x401: {
        if (frame.can_dlc < 2) break;
        int flags = static_cast<int>(frame.data[0])
                  | (static_cast<int>(frame.data[1]) << 8);
        if (flags != warning_flags_) {
            warning_flags_ = flags;
            emit warningFlagsChanged(warning_flags_);
        }
        break;
    }

    // ── 0x500 VEHICLE_STATE: [속도u16×10 BE][RPM u16 BE][기어][ABS/TCS] ──
    case 0x500: {
        if (frame.can_dlc < 6) break;
        // 속도·RPM은 0x400이 primary — 여기선 기어와 ABS/TCS만 갱신
        int gear  = static_cast<int>(frame.data[4]);
        bool abs_a = (frame.data[5] & 0x01) != 0;
        bool tcs_a = (frame.data[5] & 0x02) != 0;
        if (gear  != gear_)       { gear_       = gear;  emit gearChanged(gear_); }
        if (abs_a != abs_active_) { abs_active_ = abs_a; emit absActiveChanged(abs_active_); }
        if (tcs_a != tcs_active_) { tcs_active_ = tcs_a; emit tcsActiveChanged(tcs_active_); }
        break;
    }

    // ── 0x501 ENGINE_STATE: [수온°C][유압 0~100][연료%] ──────────────────────
    case 0x501: {
        if (frame.can_dlc < 3) break;
        int temp = static_cast<int>(frame.data[0]);
        int oil  = static_cast<int>(frame.data[1]);
        int fuel = static_cast<int>(frame.data[2]);
        if (temp != temperature_)  { temperature_  = temp; emit temperatureChanged(temperature_); }
        if (oil  != oil_pressure_) { oil_pressure_ = oil;  emit oilPressureChanged(oil_pressure_); }
        if (fuel != fuel_level_)   { fuel_level_   = fuel; emit fuelLevelChanged(fuel_level_); }
        break;
    }

    default:
        break;
    }
}
