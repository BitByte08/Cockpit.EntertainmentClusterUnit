#ifndef VEHICLE_INFO_WIDGET_HPP
#define VEHICLE_INFO_WIDGET_HPP

#include <QWidget>
#include <QLabel>
#include <cstdint>

class EntertainmentModel;

/// 차량 주행 정보 페이지 (페이지 2)
/// - 클러스터에 없는 데이터만 표시
/// - 0x502: 전달 토크, 횡G, 종G
/// - 0x503: ABS/TCS 상태, 휠락 FL/FR/RL/RR
class VehicleInfoWidget : public QWidget {
    Q_OBJECT
public:
    explicit VehicleInfoWidget(QWidget *parent = nullptr);
    void setModel(EntertainmentModel *model);

public slots:
    void onDrivingDynamicsChanged(int torque, double latG, double lonG);
    void onAdasStatusChanged(bool absActive, bool tcsActive, uint8_t wheelLock);
    void onRpmChanged(int rpm);   // 엔진 실행 상태 판단용

protected:
    void paintEvent(QPaintEvent *) override;

private:
    EntertainmentModel *model_{nullptr};

    // 토크 / G-force 레이블
    QLabel *lbl_torque_{nullptr};
    QLabel *lbl_lat_g_{nullptr};
    QLabel *lbl_lon_g_{nullptr};

    // ADAS 상태 레이블
    QLabel *lbl_abs_{nullptr};
    QLabel *lbl_tcs_{nullptr};

    // 휠락 인디케이터 (FL FR RL RR)
    QWidget *ind_fl_{nullptr};
    QWidget *ind_fr_{nullptr};
    QWidget *ind_rl_{nullptr};
    QWidget *ind_rr_{nullptr};

    // G-force 바
    QWidget *bar_lat_pos_{nullptr};   // 우측 횡G 바
    QWidget *bar_lat_neg_{nullptr};   // 좌측 횡G 바
    QWidget *bar_lon_pos_{nullptr};   // 가속 종G 바
    QWidget *bar_lon_neg_{nullptr};   // 감속 종G 바
    QWidget *track_lat_{nullptr};
    QWidget *track_lon_{nullptr};

    int     rpm_{0};
    int     torque_{0};
    double  lat_g_{0.0};
    double  lon_g_{0.0};
    bool    abs_active_{false};
    bool    tcs_active_{false};
    uint8_t wheel_lock_{0};

    void buildUI();
    void updateGBar(QWidget *posBar, QWidget *negBar, QWidget *track, double val, double maxVal);
    void setWheelLock(QWidget *ind, bool locked);
};

#endif // VEHICLE_INFO_WIDGET_HPP
