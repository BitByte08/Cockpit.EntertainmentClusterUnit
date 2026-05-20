#ifndef VEHICLE_INFO_WIDGET_HPP
#define VEHICLE_INFO_WIDGET_HPP

#include <QWidget>
#include <QLabel>
#include <cstdint>

class EntertainmentModel;

/// 차량 텔레메트리 페이지 (페이지 2)
/// CAN 수신 데이터: 속도·RPM (0x400), 기어 (0x500), 수온·유압·연료 (0x501)
class VehicleInfoWidget : public QWidget {
    Q_OBJECT
public:
    explicit VehicleInfoWidget(QWidget *parent = nullptr);
    void setModel(EntertainmentModel *model);

public slots:
    void onSpeedChanged(int kmh);
    void onRpmChanged(int rpm);
    void onGearChanged(int gear);
    void onEngineStateChanged(int coolant, int oilPct, int fuelPct);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    EntertainmentModel *model_{nullptr};

    // 수치 레이블
    QLabel *lbl_speed_val_{nullptr};
    QLabel *lbl_rpm_val_{nullptr};
    QLabel *lbl_gear_val_{nullptr};
    QLabel *lbl_fuel_val_{nullptr};
    QLabel *lbl_coolant_val_{nullptr};
    QLabel *lbl_oil_val_{nullptr};

    // 연료·수온 게이지 바 (QWidget, width를 비율로 변경)
    QWidget *bar_fuel_{nullptr};
    QWidget *bar_coolant_{nullptr};

    int speed_{0};
    int rpm_{0};
    int gear_{0};
    int fuel_pct_{0};
    int coolant_{0};
    int oil_pct_{0};

    void buildUI();
    void updateBar(QWidget *bar, QWidget *track, int pct);

    QWidget *fuel_track_{nullptr};
    QWidget *coolant_track_{nullptr};
};

#endif // VEHICLE_INFO_WIDGET_HPP
