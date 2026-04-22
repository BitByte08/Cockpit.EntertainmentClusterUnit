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
    void stop()  override {}

    void inject(uint32_t id, uint8_t dlc,
                uint8_t b0=0, uint8_t b1=0,
                uint8_t b2=0, uint8_t b3=0,
                uint8_t b4=0, uint8_t b5=0) {
        can_frame f{};
        f.can_id  = id;
        f.can_dlc = dlc;
        f.data[0] = b0; f.data[1] = b1;
        f.data[2] = b2; f.data[3] = b3;
        f.data[4] = b4; f.data[5] = b5;
        emit frameReceived(f);
    }
};

// ── 테스트 클래스 ─────────────────────────────────────────────────────────────
class ClusterModelTest : public QObject {
    Q_OBJECT

    ClusterModel      *model_{nullptr};
    TestCANInterface  *iface_{nullptr};

private slots:
    void init() {
        model_ = new ClusterModel();
        iface_ = new TestCANInterface();
        model_->setCANInterface(std::unique_ptr<CANInterface>(iface_));
    }
    void cleanup() {
        delete model_;
        model_ = nullptr;
        iface_ = nullptr;
    }

    // ── 0x400 INFO_SPEED_RPM ─────────────────────────────────────────────────

    void testSpeedAndRpmParsed() {
        QSignalSpy spySpd(model_, &ClusterModel::speedChanged);
        QSignalSpy spyRpm(model_, &ClusterModel::rpmChanged);
        // 속도: 870 (87 km/h × 10) = 0x0366, RPM: 3000 = 0x0BB8
        iface_->inject(0x400, 4, 0x03, 0x66, 0x0B, 0xB8);
        QCOMPARE(spySpd.count(), 1);
        QCOMPARE(spySpd.takeFirst().at(0).toInt(), 87);
        QCOMPARE(spyRpm.count(), 1);
        QCOMPARE(spyRpm.takeFirst().at(0).toInt(), 3000);
    }

    void testSpeedDividesByTen() {
        QSignalSpy spy(model_, &ClusterModel::speedChanged);
        // 속도: 1234 = 0x04D2 → 123 km/h (정수 나눗셈)
        iface_->inject(0x400, 4, 0x04, 0xD2, 0x00, 0x00);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), 123);
    }

    void testSpeedRequiresDlc4() {
        QSignalSpy spySpd(model_, &ClusterModel::speedChanged);
        QSignalSpy spyRpm(model_, &ClusterModel::rpmChanged);
        iface_->inject(0x400, 3, 0x01, 0x00, 0x00); // DLC < 4 → 무시
        QCOMPARE(spySpd.count(), 0);
        QCOMPARE(spyRpm.count(), 0);
    }

    void testSpeedNoDuplicateSignal() {
        iface_->inject(0x400, 4, 0x03, 0xE8, 0x00, 0x00); // 초기 설정 (100 km/h)
        QSignalSpy spy(model_, &ClusterModel::speedChanged);
        iface_->inject(0x400, 4, 0x03, 0xE8, 0x00, 0x00); // 동일값 → 시그널 없음
        QCOMPARE(spy.count(), 0);
    }

    // ── 0x301 GEAR_STATUS ────────────────────────────────────────────────────

    void testGearParsed() {
        QSignalSpy spy(model_, &ClusterModel::gearChanged);
        iface_->inject(0x301, 1, 3); // 3단
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), 3);
    }

    void testGearReverse() {
        QSignalSpy spy(model_, &ClusterModel::gearChanged);
        iface_->inject(0x301, 1, 7); // 후진 (R)
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), 7);
    }

    void testGearNeutral() {
        iface_->inject(0x301, 1, 1); // 먼저 1단으로 설정
        QSignalSpy spy(model_, &ClusterModel::gearChanged);
        iface_->inject(0x301, 1, 0); // 중립
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), 0);
    }

    // ── 0x300 SWITCH_STATUS ─────────────────────────────────────────────────

    void testSwitchStatusParsed() {
        QSignalSpy spy(model_, &ClusterModel::switchStatusChanged);
        // Ignition (bit0) + Engine (bit1) = 0x03, low byte=0x03, high byte=0x00
        iface_->inject(0x300, 2, 0x03, 0x00);
        QCOMPARE(spy.count(), 1);
        int flags = spy.takeFirst().at(0).toInt();
        QVERIFY(flags & SwitchBit::Ignition);
        QVERIFY(flags & SwitchBit::Engine);
    }

    void testTurnLeftFlagDetected() {
        QSignalSpy spy(model_, &ClusterModel::switchStatusChanged);
        // TurnLeft = bit8 = 0x0100 → low byte=0x00, high byte=0x01
        iface_->inject(0x300, 2, 0x00, 0x01);
        QCOMPARE(spy.count(), 1);
        int flags = spy.takeFirst().at(0).toInt();
        QVERIFY(flags & SwitchBit::TurnLeft);
        QVERIFY(!(flags & SwitchBit::TurnRight));
    }

    void testHazardFlagDetected() {
        QSignalSpy spy(model_, &ClusterModel::switchStatusChanged);
        // Hazard = bit4 = 0x10
        iface_->inject(0x300, 2, 0x10, 0x00);
        QCOMPARE(spy.count(), 1);
        int flags = spy.takeFirst().at(0).toInt();
        QVERIFY(flags & SwitchBit::Hazard);
    }

    void testHighBeamFlagDetected() {
        QSignalSpy spy(model_, &ClusterModel::switchStatusChanged);
        // HighBeam = bit3 = 0x08
        iface_->inject(0x300, 2, 0x08, 0x00);
        QCOMPARE(spy.count(), 1);
        QVERIFY(spy.takeFirst().at(0).toInt() & SwitchBit::HighBeam);
    }

    // ── 0x401 INFO_WARNING ───────────────────────────────────────────────────

    void testWarningFlagsParsed() {
        QSignalSpy spy(model_, &ClusterModel::warningFlagsChanged);
        // CheckEngine (bit0) + OilPressure (bit1) = 0x03
        iface_->inject(0x401, 2, 0x03, 0x00);
        QCOMPARE(spy.count(), 1);
        int flags = spy.takeFirst().at(0).toInt();
        QVERIFY(flags & WarningBit::CheckEngine);
        QVERIFY(flags & WarningBit::OilPressure);
    }

    // ── 0x500 VEHICLE_STATE ─────────────────────────────────────────────────

    void testVehicleStateAbsTcs() {
        QSignalSpy spyAbs(model_, &ClusterModel::absActiveChanged);
        QSignalSpy spyTcs(model_, &ClusterModel::tcsActiveChanged);
        // 기어=2, flags=0x03 (ABS+TCS 모두 활성)
        iface_->inject(0x500, 6, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03);
        QCOMPARE(spyAbs.count(), 1);
        QVERIFY(spyAbs.takeFirst().at(0).toBool());
        QCOMPARE(spyTcs.count(), 1);
        QVERIFY(spyTcs.takeFirst().at(0).toBool());
    }

    void testVehicleStateGear() {
        QSignalSpy spy(model_, &ClusterModel::gearChanged);
        iface_->inject(0x500, 6, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00); // 5단
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), 5);
    }

    void testVehicleStateRequiresDlc6() {
        QSignalSpy spy(model_, &ClusterModel::absActiveChanged);
        iface_->inject(0x500, 5, 0x00, 0x00, 0x00, 0x00, 0x01); // DLC < 6 → 무시
        QCOMPARE(spy.count(), 0);
    }

    // ── 0x501 ENGINE_STATE ───────────────────────────────────────────────────

    void testEngineStateParsed() {
        QSignalSpy spyTemp(model_, &ClusterModel::temperatureChanged);
        QSignalSpy spyOil (model_, &ClusterModel::oilPressureChanged);
        QSignalSpy spyFuel(model_, &ClusterModel::fuelLevelChanged);
        // 수온=95°C, 유압=60, 연료=72%
        iface_->inject(0x501, 3, 95, 60, 72);
        QCOMPARE(spyTemp.count(), 1);
        QCOMPARE(spyTemp.takeFirst().at(0).toInt(), 95);
        QCOMPARE(spyOil.count(), 1);
        QCOMPARE(spyOil.takeFirst().at(0).toInt(), 60);
        QCOMPARE(spyFuel.count(), 1);
        QCOMPARE(spyFuel.takeFirst().at(0).toInt(), 72);
    }

    void testEngineStateRequiresDlc3() {
        QSignalSpy spy(model_, &ClusterModel::temperatureChanged);
        iface_->inject(0x501, 2, 95, 60); // DLC < 3 → 무시
        QCOMPARE(spy.count(), 0);
    }

    // ── 구형 CAN ID는 무시됨 ─────────────────────────────────────────────────

    void testOldCanIdsIgnored() {
        QSignalSpy s1(model_, &ClusterModel::speedChanged);
        QSignalSpy s2(model_, &ClusterModel::rpmChanged);
        QSignalSpy s3(model_, &ClusterModel::fuelLevelChanged);
        QSignalSpy s4(model_, &ClusterModel::temperatureChanged);

        // 구형 ID 주입 (0x100=스티어링, 0x200=페달, 이제 클러스터 데이터 아님)
        iface_->inject(0x100, 2, 0x00, 0x57);
        iface_->inject(0x200, 2, 0x0B, 0xB8);

        QCOMPARE(s1.count(), 0);
        QCOMPARE(s2.count(), 0);
        QCOMPARE(s3.count(), 0);
        QCOMPARE(s4.count(), 0);
    }

    void testUnknownIdIgnored() {
        QSignalSpy s1(model_, &ClusterModel::speedChanged);
        QSignalSpy s2(model_, &ClusterModel::switchStatusChanged);
        iface_->inject(0x999, 4, 0xFF, 0xFF, 0xFF, 0xFF);
        QCOMPARE(s1.count(), 0);
        QCOMPARE(s2.count(), 0);
    }

    // ── 초기값 ───────────────────────────────────────────────────────────────

    void testInitialValues() {
        QCOMPARE(model_->getSpeed(),       0);
        QCOMPARE(model_->getRpm(),         0);
        QCOMPARE(model_->getGear(),        0);
        QCOMPARE(model_->getFuelLevel(),   100);
        QCOMPARE(model_->getTemperature(), 90);
        QCOMPARE(model_->getOilPressure(), 50);
        QCOMPARE(model_->getSwitchStatus(),  0);
        QCOMPARE(model_->getWarningFlags(),  0);
        QVERIFY(!model_->isAbsActive());
        QVERIFY(!model_->isTcsActive());
    }
};

QTEST_MAIN(ClusterModelTest)
#include "ClusterModelTest.moc"
