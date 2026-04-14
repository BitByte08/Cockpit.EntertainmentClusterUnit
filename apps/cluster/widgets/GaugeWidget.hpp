#ifndef CLUSTER_GAUGEWIDGET_HPP
#define CLUSTER_GAUGEWIDGET_HPP

#include <QWidget>
#include <QString>

class GaugeWidget : public QWidget {
    Q_OBJECT
public:
    explicit GaugeWidget(QWidget *parent = nullptr);
    ~GaugeWidget() override = default;

    void setValue(int value);
    void setMinValue(int min) { min_value_ = min; update(); }
    void setMaxValue(int max) { max_value_ = max; update(); }
    void setUnit(const QString &unit) { unit_ = unit; update(); }
    void setLabel(const QString &label) { label_ = label; update(); }
    void setMajorTicks(int ticks) { major_ticks_ = ticks; update(); }
    void setRedZoneStart(int value) { red_zone_start_ = value; update(); }
    
    int getValue() const { return current_value_; }

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    void drawBackground(QPainter &painter);
    void drawScale(QPainter &painter);
    void drawNeedle(QPainter &painter);
    void drawValue(QPainter &painter);
    void drawLabel(QPainter &painter);

    int current_value_{0};
    int target_value_{0};
    int min_value_{0};
    int max_value_{200};
    int major_ticks_{10};
    int red_zone_start_{180};
    QString unit_{"km/h"};
    QString label_{"SPEED"};
    
    // Animation
    int animation_step_{0};
    static constexpr int ANIMATION_STEPS = 10;
};

#endif // CLUSTER_GAUGEWIDGET_HPP
