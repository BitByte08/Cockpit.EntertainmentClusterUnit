#include "FuelBarWidget.hpp"
#include <QPainter>

static const QColor kFuelLit   {0xFF, 0xFF, 0xFF};
static const QColor kFuelRed   {0xE2, 0x27, 0x18};
static const QColor kFuelDark  {0x26, 0x26, 0x26};

FuelBarWidget::FuelBarWidget(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
}

void FuelBarWidget::setPercent(int pct) {
    pct_ = qBound(0, pct, 100);
    update();
}

void FuelBarWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    const int    w    = width();
    const int    h    = height();
    const int    gap  = 2;
    const double cw   = (static_cast<double>(w) - gap * (cells_ - 1)) / cells_;
    const int    lit  = qRound((pct_ / 100.0) * cells_);
    const bool   crit = (pct_ <= crit_pct_);

    for (int i = 0; i < cells_; i++) {
        const double x = i * (cw + gap);
        QColor c;
        if (i < lit) {
            // Reserve zone (first 2 cells) shows red when fuel is critical
            c = (crit && i < 2) ? kFuelRed : kFuelLit;
        } else {
            c = kFuelDark;
        }
        p.fillRect(QRectF(x, 0, cw, h), c);
    }
}
