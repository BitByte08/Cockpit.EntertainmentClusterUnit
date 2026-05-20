#ifndef ENTERTAINMENT_WINDOW_HPP
#define ENTERTAINMENT_WINDOW_HPP

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include "TileMapWidget.hpp"
#include "models/EntertainmentModel.hpp"

// ── 회전 안내 위젯 ──────────────────────────────────────────────────────────────
class ManeuverWidget : public QWidget {
    Q_OBJECT
public:
    explicit ManeuverWidget(QWidget *parent = nullptr);

public slots:
    void updateManeuver(TileMapWidget::Maneuver type, double distMeters);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    TileMapWidget::Maneuver type_{TileMapWidget::Maneuver::None};
    double dist_{-1.0};
};

// ── 좌측 사이드 레일 ──────────────────────────────────────────────────────────
class SideRailWidget : public QWidget {
    Q_OBJECT
public:
    explicit SideRailWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    void drawNavIcon    (QPainter &p, QRect r) const;
    void drawSettingsIcon(QPainter &p, QRect r) const;
};

// ── 상단 Status Bar ────────────────────────────────────────────────────────────
class StatusBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit StatusBarWidget(QWidget *parent = nullptr);

public slots:
    void updateClock();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    QString time_str_{"00:00"};
    QString date_str_;
};

// ── 내비게이션 화면 (타일맵 + 오버레이) ──────────────────────────────────────
class NavScreen : public QWidget {
    Q_OBJECT
public:
    explicit NavScreen(QWidget *parent = nullptr);

    TileMapWidget *tileMap() { return tile_map_; }

public slots:
    void setSpeed(int kmh);
    void onDistanceChanged(double meters);   // 경로 거리 업데이트

protected:
    void resizeEvent(QResizeEvent *) override;

private:
    void layoutOverlays();
    void buildETACard();
    void buildSpeedBadge();
    void buildSpeedLimit();
    void buildZoomCtrl();

    TileMapWidget *tile_map_{nullptr};
    ManeuverWidget  *maneuver_widget_{nullptr};
    QWidget       *eta_card_{nullptr};
    QWidget       *speed_badge_{nullptr};
    QWidget       *speed_limit_{nullptr};
    QWidget       *zoom_ctrl_{nullptr};
    QLabel        *speed_val_label_{nullptr};
    QLabel        *eta_title_label_{nullptr};
    QLabel        *eta_dist_label_{nullptr};
};

// ── 메인 엔터테인먼트 윈도우 ──────────────────────────────────────────────────
class EntertainmentWindow : public QWidget {
    Q_OBJECT
public:
    explicit EntertainmentWindow(QWidget *parent = nullptr);
    ~EntertainmentWindow() override = default;

    void setModel(EntertainmentModel *model);
    bool loadRoadGraph(const QString &jsonPath);

private:
    SideRailWidget  *rail_{nullptr};
    StatusBarWidget *status_bar_{nullptr};
    NavScreen       *nav_screen_{nullptr};

    QTimer             clock_timer_;
    EntertainmentModel *model_{nullptr};
};

#endif // ENTERTAINMENT_WINDOW_HPP
