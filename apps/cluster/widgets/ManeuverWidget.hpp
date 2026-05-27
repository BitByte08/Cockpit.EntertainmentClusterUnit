#ifndef CLUSTER_MANEUVER_WIDGET_HPP
#define CLUSTER_MANEUVER_WIDGET_HPP

#include <QWidget>

/// 내비게이션 방향 오버레이 — 클러스터 센터 패널 상단 좌측에 표시
/// CAN 0x700 수신 시 업데이트. type=0(None)이면 hide().
class ManeuverWidget : public QWidget {
    Q_OBJECT
public:
    enum class Maneuver { None = 0, Straight = 1, TurnLeft = 2, TurnRight = 3, Arrived = 4 };

    explicit ManeuverWidget(QWidget *parent = nullptr);

public slots:
    void onManeuverChanged(int type, int distMeters);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    Maneuver type_{Maneuver::None};
    int      dist_{0};
};

#endif // CLUSTER_MANEUVER_WIDGET_HPP
