#include "EntertainmentModel.hpp"
#ifndef _WIN32
#include <linux/can.h>
#endif
#include <QThread>

EntertainmentModel::EntertainmentModel(QObject *parent) : QObject(parent) {
    qRegisterMetaType<can_frame>("can_frame");
}

void EntertainmentModel::setCANInterface(std::unique_ptr<CANInterface> can) {
    can_ = std::move(can);
    connect(can_.get(), &CANInterface::frameReceived,
            this, &EntertainmentModel::onFrameReceived,
            Qt::QueuedConnection);
}

void EntertainmentModel::startReceiving() {
    if (!can_) return;
    auto *thread = new QThread(this);
    can_->moveToThread(thread);
    connect(thread, &QThread::started,  can_.get(), &CANInterface::start);
    connect(thread, &QThread::finished, thread,     &QThread::deleteLater);
    thread->start();
}

void EntertainmentModel::sendSwitchFlags(uint16_t flags) {
    if (!can_) return;
    sw_flags_ = flags;
    can_frame frame{};
    frame.can_id  = 0x300;
    frame.can_dlc = 2;
    frame.data[0] = static_cast<uint8_t>(flags & 0xFF);
    frame.data[1] = static_cast<uint8_t>((flags >> 8) & 0xFF);
    // write()는 소켓 FD에 대해 스레드 안전 — QObject moveToThread와 무관
    can_->sendFrame(frame);
    emit switchFlagsChanged(sw_flags_);
}

void EntertainmentModel::onFrameReceived(const can_frame &frame) {
    switch (frame.can_id) {

    // ── 0x101 STEERING_COLUMN: [flags uint16 LE] (OpenFFBoard → Pi) ─────────
    case 0x101: {
        if (frame.can_dlc < 2) break;
        uint16_t flags = static_cast<uint16_t>(frame.data[0])
                       | (static_cast<uint16_t>(frame.data[1]) << 8);
        if (flags != turn_flags_) { turn_flags_ = flags; emit turnFlagsChanged(turn_flags_); }
        break;
    }

    // ── 0x300 SWITCH_STATUS: [flags uint16 LE] ──────────────────────────────
    case 0x300: {
        if (frame.can_dlc < 2) break;
        uint16_t flags = static_cast<uint16_t>(frame.data[0])
                       | (static_cast<uint16_t>(frame.data[1]) << 8);
        if (flags != sw_flags_) { sw_flags_ = flags; emit switchFlagsChanged(sw_flags_); }
        break;
    }

    // ── 0x400 INFO_SPEED_RPM: [속도 u16×10 BE][RPM u16 BE] ─────────────────
    case 0x400: {
        if (frame.can_dlc < 2) break;
        int speed = ((static_cast<int>(frame.data[0]) << 8) | frame.data[1]) / 10;
        if (speed != speed_) { speed_ = speed; emit speedChanged(speed_); }
        if (frame.can_dlc >= 4) {
            int rpm = (static_cast<int>(frame.data[2]) << 8) | frame.data[3];
            if (rpm != rpm_) { rpm_ = rpm; emit rpmChanged(rpm_); }
        }
        break;
    }

    // ── 0x501 ENGINE_STATE: [coolant u8 °C][oil u8 %][fuel u8 %] ────────────
    case 0x501: {
        if (frame.can_dlc < 3) break;
        int c = frame.data[0], o = frame.data[1], f = frame.data[2];
        if (c != coolant_ || o != oil_pct_ || f != fuel_pct_) {
            coolant_  = c; oil_pct_ = o; fuel_pct_ = f;
            emit engineStateChanged(coolant_, oil_pct_, fuel_pct_);
        }
        break;
    }

    // ── 0x500 VEHICLE_STATE: byte[4] = gear ─────────────────────────────────
    case 0x500: {
        if (frame.can_dlc < 5) break;
        int gear = static_cast<int>(frame.data[4]);
        if (gear != gear_) { gear_ = gear; emit gearChanged(gear_); }
        break;
    }

    // ── 0x600 POSITION: [X int32 BE ×100][Z int32 BE ×100] ──────────────────
    case 0x600: {
        if (frame.can_dlc < 8) break;
        auto be32 = [&](int off) -> int32_t {
            return (static_cast<int32_t>(frame.data[off+0]) << 24)
                 | (static_cast<int32_t>(frame.data[off+1]) << 16)
                 | (static_cast<int32_t>(frame.data[off+2]) <<  8)
                 |  static_cast<int32_t>(frame.data[off+3]);
        };
        pos_x_ = be32(0) / 100.0;
        pos_z_ = be32(4) / 100.0;
        emit positionChanged(pos_x_, pos_z_);
        break;
    }

    // ── 0x601 HEADING: [heading uint16 BE ×10] ──────────────────────────────
    case 0x601: {
        if (frame.can_dlc < 2) break;
        uint16_t h = (static_cast<uint16_t>(frame.data[0]) << 8) | frame.data[1];
        heading_ = h / 10.0;
        emit headingChanged(heading_);
        break;
    }

    default:
        break;
    }
}
