#ifndef SWITCH_PANEL_WIDGET_HPP
#define SWITCH_PANEL_WIDGET_HPP

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <cstdint>

class EntertainmentModel;

// ── 스위치 패널 페이지 ────────────────────────────────────────────────────────
// CAN 0x300 (메인 패널) 전송 + CAN 0x101 (스티어링 컬럼) 수신 표시
class SwitchPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit SwitchPanelWidget(QWidget *parent = nullptr);
    void setModel(EntertainmentModel *model);

public slots:
    void onSwitchFlagsChanged(uint16_t flags);
    void onTurnFlagsChanged(uint16_t flags);
    void onRpmChanged(int rpm);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    EntertainmentModel *model_{nullptr};
    uint16_t sw_flags_{0};
    uint16_t turn_flags_{0};
    int      rpm_{0};

    QPushButton *btn_start_{nullptr};      // 이그니션+시동 통합
    QPushButton *btn_lights_{nullptr};     // OFF→LOW→HIGH→OFF 순환
    QPushButton *btn_wiper_{nullptr};      // OFF→SLOW→FAST→OFF 순환
    QPushButton *btn_turn_left_{nullptr};  // 좌 방향지시등 (버튼 → 토글)
    QPushButton *btn_hazard_{nullptr};     // 비상등
    QPushButton *btn_turn_right_{nullptr}; // 우 방향지시등 (버튼 → 토글)
    QPushButton *btn_horn_{nullptr};

    void buildUI();
    void toggleBit(uint16_t mask);
    void setBit(uint16_t mask, bool on);
    void sendFlags();
    void refreshStyles();
};

#endif // SWITCH_PANEL_WIDGET_HPP
