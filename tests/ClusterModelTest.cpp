#include <QTest>
#include <QSignalSpy>
#include <QObject>

#include "models/ClusterModel.hpp"
#include "CANInterface.hpp"
#ifdef _WIN32
#include "can_frame_stub.hpp"
#else
#include <linux/can.h>
#endif

// ── 테스트용 CAN 인터페이스 ──────────────────────────────────────────────────
class TestCANInterface : public CANInterface {
    Q_OBJECT
public:
    explicit TestCANInterface() : CANInterface(nullptr) {}
    void start() override {}
    void stop() override {}

    void inject(uint32_t id, uint8_t dlc,
                uint8_t b0 = 0, uint8_t b1 = 0,
                uint8_t b2 = 0, uint8_t b3 = 0) {
        can_frame f{};
        f.can_id  = id;
        f.can_dlc = dlc;
        f.data[0] = b0; f.data[1] = b1;
        f.data[2] = b2; f.data[3] = b3;
        emit frameReceived(f);
    }
};

// ── 테스트 클래스 ─────────────────────────────────────────────────────────────
class ClusterModelTest : public QObject {
    Q_OBJECT

private:
    ClusterModel *model_{nullptr};
    TestCANInterface *iface_{nullptr};

private slots:
    // ── 픽스처 ──────────────────────────────────────────────────────────────
    void init() {
        model_ = new ClusterModel();
        iface_ = new TestCANInterface();
        model_->setCANInterface(std::unique_ptr<CANInterface>(iface_));
    }

    void cleanup() {
        delete model_;
        model_ = nullptr;
        // iface_는 model_ 소멸 시 unique_ptr이 해제함
        iface_ = nullptr;
    }

    // ── 속도 파싱 ────────────────────────────────────────────────────────────
    void testSpeedParsed() {
        QSignalSpy spy(model_, &ClusterModel::speedChanged);
        // 87 km/h → 0x00, 0x57
        iface_->inject(0x100, 2, 0x00, 0x57);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), 87);
    }

    void testSpeedHighByte() {
        QSignalSpy spy(model_, &ClusterModel::speedChanged);
        // 256 km/h → 0x01, 0x00
        iface_->inject(0x100, 2, 0x01, 0x00);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), 256);
    }

    void testSpeedRequiresDlc2() {
        QSignalSpy spy(model_, &ClusterModel::speedChanged);
        // DLC < 2 → 무시
        iface_->inject(0x100, 1, 0x55);
        QCOMPARE(spy.count(), 0);
    }

    void testSpeedNoDuplicateSignal() {
        iface_->inject(0x100, 2, 0x00, 0x64);   // 100 km/h
        QSignalSpy spy(model_, &ClusterModel::speedChanged);
        iface_->inject(0x100, 2, 0x00, 0x64);   // 동일 값 재전송
        QCOMPARE(spy.count(), 0);                // 중복 시그널 없음
    }

    // ── RPM 파싱 ─────────────────────────────────────────────────────────────
    void testRpmParsed() {
        QSignalSpy spy(model_, &ClusterModel::rpmChanged);
        // 3000 RPM → 0x0B, 0xB8
        iface_->inject(0x200, 2, 0x0B, 0xB8);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), 3000);
    }

    void testRpmZero() {
        QSignalSpy spy(model_, &ClusterModel::rpmChanged);
        iface_->inject(0x200, 2, 0x00, 0x00);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), 0);
    }

    // ── 연료 파싱 ────────────────────────────────────────────────────────────
    void testFuelParsed() {
        QSignalSpy spy(model_, &ClusterModel::fuelLevelChanged);
        iface_->inject(0x300, 1, 72);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), 72);
    }

    void testFuelRequiresDlc1() {
        QSignalSpy spy(model_, &ClusterModel::fuelLevelChanged);
        iface_->inject(0x300, 0);  // DLC == 0 → 무시
        QCOMPARE(spy.count(), 0);
    }

    // ── 온도 파싱 ────────────────────────────────────────────────────────────
    void testTempParsed() {
        QSignalSpy spy(model_, &ClusterModel::temperatureChanged);
        iface_->inject(0x400, 1, 95);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), 95);
    }

    // ── 알 수 없는 CAN ID ────────────────────────────────────────────────────
    void testUnknownIdIgnored() {
        QSignalSpy s1(model_, &ClusterModel::speedChanged);
        QSignalSpy s2(model_, &ClusterModel::rpmChanged);
        QSignalSpy s3(model_, &ClusterModel::fuelLevelChanged);
        QSignalSpy s4(model_, &ClusterModel::temperatureChanged);

        iface_->inject(0x999, 2, 0xFF, 0xFF);

        QCOMPARE(s1.count(), 0);
        QCOMPARE(s2.count(), 0);
        QCOMPARE(s3.count(), 0);
        QCOMPARE(s4.count(), 0);
    }

    // ── 초기값 ───────────────────────────────────────────────────────────────
    void testInitialValues() {
        QCOMPARE(model_->getSpeed(), 0);
        QCOMPARE(model_->getRpm(), 0);
        QCOMPARE(model_->getFuelLevel(), 100);
        QCOMPARE(model_->getTemperature(), 90);
    }
};

QTEST_MAIN(ClusterModelTest)
#include "ClusterModelTest.moc"
