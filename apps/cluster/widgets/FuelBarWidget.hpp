#ifndef FUELBARWIDGET_HPP
#define FUELBARWIDGET_HPP

#include <QWidget>

// BMW M Design — discrete-cell fuel level bar.
// Mirrors the design-system `FuelBar` component from cluster-shared.jsx.
// 16 cells: first 2 are the "reserve zone" (shown red when lit and fuel is low).
class FuelBarWidget : public QWidget {
    Q_OBJECT
public:
    explicit FuelBarWidget(QWidget *parent = nullptr);

    void setCells(int n)    { cells_    = qMax(1, n); update(); }
    void setCritPct(int pct){ crit_pct_ = pct;        update(); }

    QSize sizeHint() const override { return {160, 8}; }
    QSize minimumSizeHint() const override { return {60, 6}; }

public slots:
    void setPercent(int pct);

private:
    void paintEvent(QPaintEvent *) override;

    int pct_{100};
    int cells_{16};
    int crit_pct_{15};   // ≤ this = first 2 cells shown in red
};

#endif // FUELBARWIDGET_HPP
