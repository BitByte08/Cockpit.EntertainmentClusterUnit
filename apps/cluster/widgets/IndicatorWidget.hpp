#ifndef CLUSTER_INDICATORWIDGET_HPP
#define CLUSTER_INDICATORWIDGET_HPP

#include <QWidget>
#include <QString>
#include <QColor>
#include <QTimer>

enum class IndicatorIcon {
    TurnLeft, TurnRight, HighBeam, LowBeam,
    CheckEngine, OilPressure, ABS, TCS, FuelWarn, Battery,
};

class IndicatorWidget : public QWidget {
    Q_OBJECT
public:
    explicit IndicatorWidget(IndicatorIcon icon,
                              const QString &label,
                              const QColor  &activeColor,
                              QWidget       *parent = nullptr);
    ~IndicatorWidget() override = default;

    void setActive(bool active);
    void startBlink();
    void stopBlink();
    bool isActive() const { return active_; }

    QSize sizeHint()        const override { return QSize(52, 46); }
    QSize minimumSizeHint() const override { return QSize(46, 40); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawIcon (QPainter &p, const QColor &c, int iw, int ih);
    void drawTurnArrow   (QPainter &p, bool left,   const QColor &c, int iw, int ih);
    void drawHighBeam    (QPainter &p, const QColor &c, int iw, int ih);
    void drawCheckEngine (QPainter &p, const QColor &c, int iw, int ih);
    void drawOilPressure (QPainter &p, const QColor &c, int iw, int ih);
    void drawTextBox     (QPainter &p, const QString &text, const QColor &c, int iw, int ih);
    void drawFuelWarn    (QPainter &p, const QColor &c, int iw, int ih);
    void drawBattery     (QPainter &p, const QColor &c, int iw, int ih);

    IndicatorIcon icon_;
    QString       label_;
    QColor        active_color_;
    bool          active_{false};
    bool          blinking_{false};

    static QTimer *s_shared_timer_;
    static bool    s_blink_state_;
    static int     s_ref_count_;
    static void    ensureSharedTimer();

    static constexpr QColor kDimColor{0x26, 0x26, 0x26};
};

#endif
