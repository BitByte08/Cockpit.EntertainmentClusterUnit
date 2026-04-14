#include "ClusterModel.hpp"
#ifndef _WIN32
#include <linux/can.h>
#endif
#include <QThread>
#include <iostream>

ClusterModel::ClusterModel(QObject *parent) : QObject(parent) {
    // can_frame을 Qt 메타타입으로 등록 (스레드 간 전달을 위해)
    qRegisterMetaType<can_frame>("can_frame");
}

void ClusterModel::setCANInterface(std::unique_ptr<CANInterface> can_interface) {
    can_interface_ = std::move(can_interface);
    connect(can_interface_.get(), &CANInterface::frameReceived,
            this, &ClusterModel::onFrameReceived);
}

void ClusterModel::startReceiving() {
    if (!can_interface_) return;
    
    // CAN interface를 별도 스레드에서 실행
    QThread *thread = new QThread;
    can_interface_->moveToThread(thread);
    
    connect(thread, &QThread::started, can_interface_.get(), &CANInterface::start);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    
    thread->start();
}

void ClusterModel::stopReceiving() {
    if (can_interface_) {
        can_interface_->stop();
    }
}

void ClusterModel::onFrameReceived(const can_frame &frame) {
    parseCANFrame(frame);
}

void ClusterModel::parseCANFrame(const can_frame &frame) {
    // CAN ID에 따라 데이터 파싱
    // 예시: 0x100 = 속도, 0x200 = RPM, 0x300 = 연료, 0x400 = 온도
    
    switch (frame.can_id) {
        case 0x100: // 속도 (km/h)
            if (frame.can_dlc >= 2) {
                int new_speed = (frame.data[0] << 8) | frame.data[1];
                if (new_speed != speed_) {
                    speed_ = new_speed;
                    emit speedChanged(speed_);
                }
            }
            break;
            
        case 0x200: // RPM
            if (frame.can_dlc >= 2) {
                int new_rpm = (frame.data[0] << 8) | frame.data[1];
                if (new_rpm != rpm_) {
                    rpm_ = new_rpm;
                    emit rpmChanged(rpm_);
                }
            }
            break;
            
        case 0x300: // 연료 레벨 (0-100%)
            if (frame.can_dlc >= 1) {
                int new_fuel = frame.data[0];
                if (new_fuel != fuel_level_) {
                    fuel_level_ = new_fuel;
                    emit fuelLevelChanged(fuel_level_);
                }
            }
            break;
            
        case 0x400: // 온도 (섭씨)
            if (frame.can_dlc >= 1) {
                int new_temp = frame.data[0];
                if (new_temp != temperature_) {
                    temperature_ = new_temp;
                    emit temperatureChanged(temperature_);
                }
            }
            break;
            
        default:
            std::cout << "Unknown CAN ID: 0x" << std::hex << frame.can_id << std::dec << std::endl;
            break;
    }
}
