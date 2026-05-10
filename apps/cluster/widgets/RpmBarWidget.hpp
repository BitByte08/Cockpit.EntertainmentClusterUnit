#ifndef RPMBARWIDGET_HPP
#define RPMBARWIDGET_HPP

#include <QWidget>

// BMW M Design — discrete-cell RPM bar.
// Mirrors the design-system `RpmBar` component from cluster-shared.jsx.
// Cells light up from left to right; color: white → warn (#f4b400) → red (#e22718).
class RpmBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit RpmBarWidget(QWidget *parent = nullptr);

    void setMax(int max)      { max_     = qMax(1, max); update(); }
    void setRedline(int r)    { redline_ = r;            update(); }
    void setCells(int n)      { cells_   = qMax(1, n);  update(); }

    QSize sizeHint() const override { return {400, 28}; }
    QSize minimumSizeHint() const override { return {100, 12}; }

public slots:
    void setRpm(int rpm);

private:
    void paintEvent(QPaintEvent *) override;

    int rpm_{0};
    int max_{8000};
    int redline_{7000};
    int cells_{48};
};

#endif // RPMBARWIDGET_HPP
