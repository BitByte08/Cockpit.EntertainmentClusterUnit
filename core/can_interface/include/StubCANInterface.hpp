#ifndef CORE_STUBCANINTERFACE_HPP
#define CORE_STUBCANINTERFACE_HPP

#include "CANInterface.hpp"

// Windows 빌드용 no-op CAN 인터페이스 — CAN 프레임을 수신하지 않음
class StubCANInterface : public CANInterface {
    Q_OBJECT
public:
    explicit StubCANInterface() : CANInterface(nullptr) {}
    ~StubCANInterface() override = default;

    void start() override {}
    void stop()  override {}
};

#endif // CORE_STUBCANINTERFACE_HPP
