#ifndef CORE_CANINTERFACE_H
#define CORE_CANINTERFACE_H

#include <QObject>
#ifdef _WIN32
#include "can_frame_stub.hpp"
#else
#include <linux/can.h>
#endif

// can_frame을 Qt 메타타입으로 등록
Q_DECLARE_METATYPE(can_frame)

class CANInterface : public QObject {
    Q_OBJECT
public:
    explicit CANInterface(QObject *parent = nullptr) : QObject(parent) {};
    virtual ~CANInterface();
    virtual void start() = 0;
    virtual void stop() = 0;
signals:
    void frameReceived(const can_frame &frame);
};

#endif // CORE_CANINTERFACE_H
