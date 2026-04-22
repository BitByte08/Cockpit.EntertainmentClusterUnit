#ifndef CLUSTER_GAUGEWIDGET_HPP
#define CLUSTER_GAUGEWIDGET_HPP

#include <QWidget>
#include <QString>

/// 아날로그 원형 RPM 게이지
/// - 225° ~ -45° (시계방향 270° 스팬)
/// - 레드존 아크, 그라디언트 바늘, 부드러운 애니메이션
class GaugeWidget : public QWidget {
    Q_OBJECT
public:
    explicit GaugeWidget(QWidget *parent = nullptr);
    ~GaugeWidget() override = default;

    void setValue(int value);
    void setMinValue(int min)     { min_value_ = min; update(); }
    void setMaxValue(int max)     { max_value_ = max; update(); }
    void setUnit(const QString &u){ unit_ = u; update(); }
    void setLabel(const QString &l){ label_ = l; update(); }
    void setMajorTicks(int t)     { major_ticks_ = t; update(); }
    void setRedZoneStart(int v)   { red_zone_start_ = v; update(); }

    int getValue() const { return current_value_; }

    QSize sizeHint()        const override { return QSize(300, 300); }
    QSize minimumSizeHint() const override { return QSize(180, 180); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawBackground (QPainter &p, int side);
    void drawScale      (QPainter &p, int side);
    void drawNeedle     (QPainter &p, int side);
    void drawCenter     (QPainter &p, int side);
    void drawDigital    (QPainter &p, int side);

    int current_value_{0};
    int target_value_{0};
    int min_value_{0};
    int max_value_{8000};
    int major_ticks_{8};
    int red_zone_start_{6500};
    QString unit_{"RPM"};
    QString label_{"ENGINE RPM"};
};

#endif // CLUSTER_GAUGEWIDGET_HPP
