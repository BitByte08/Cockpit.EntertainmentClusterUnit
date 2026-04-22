#ifndef CLUSTER_ARCGAUGEWIDGET_HPP
#define CLUSTER_ARCGAUGEWIDGET_HPP

#include <QWidget>
#include <QString>

/// 반원형 아크 게이지 위젯 (연료·수온 전용)
/// - 240° 범위 아크 (좌하→우하, 상단 통과)
/// - 값에 따라 색상 그라디언트 (녹→황→적)
/// - 부드러운 애니메이션 (16ms 타이머)
class ArcGaugeWidget : public QWidget {
    Q_OBJECT
public:
    explicit ArcGaugeWidget(QWidget *parent = nullptr);
    ~ArcGaugeWidget() override = default;

    void setValue(int value);
    void setRange(int min, int max)   { min_ = min; max_ = max; update(); }
    void setLabel(const QString &lbl) { label_ = lbl; update(); }
    void setUnit(const QString &unit) { unit_  = unit; update(); }

    /// threshold: 경고 임계값
    /// highIsWarn=true  → 값 >= threshold 이면 경고 (수온)
    /// highIsWarn=false → 값 <= threshold 이면 경고 (연료)
    void setWarnThreshold(int threshold, bool highIsWarn = false) {
        warn_threshold_ = threshold;
        high_is_warn_   = highIsWarn;
    }

    int getValue() const { return current_; }

    QSize sizeHint()        const override { return QSize(220, 190); }
    QSize minimumSizeHint() const override { return QSize(160, 140); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor arcColor(double ratio) const;

    int current_{0};
    int target_{0};
    int min_{0};
    int max_{100};
    int warn_threshold_{-1};
    bool high_is_warn_{false};
    QString label_{"VALUE"};
    QString unit_{""};
};

#endif // CLUSTER_ARCGAUGEWIDGET_HPP
