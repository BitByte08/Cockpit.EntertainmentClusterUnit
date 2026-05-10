#include "RpmBarWidget.hpp"
#include <QPainter>

static const QColor kCellDark  {0x1A, 0x1A, 0x1A};
static const QColor kCellWhite {0xFF, 0xFF, 0xFF};
static const QColor kCellWarn  {0xF4, 0xB4, 0x00};
static const QColor kCellRed   {0xE2, 0x27, 0x18};

RpmBarWidget::RpmBarWidget(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, true);
}

void RpmBarWidget::setRpm(int rpm) {
    rpm_ = qBound(0, rpm, max_);
    update();
}

void RpmBarWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    // No antialiasing — crisp pixel-perfect cells (BMW M discipline)
    p.setRenderHint(QPainter::Antialiasing, false);

    const int w   = width();
    const int h   = height();
    const int gap = 2;

    const double cellW = (static_cast<double>(w) - gap * (cells_ - 1)) / cells_;
    const int    lit   = qRound((static_cast<double>(rpm_) / max_) * cells_);

    for (int i = 0; i < cells_; i++) {
        const double x       = i * (cellW + gap);
        const double cellRpm = ((i + 1.0) / cells_) * max_;

        QColor c;
        if (i < lit) {
            if (cellRpm > redline_)
                c = kCellRed;
            else if (cellRpm > redline_ * 0.85)
                c = kCellWarn;
            else
                c = kCellWhite;
        } else {
            c = kCellDark;
        }

        p.fillRect(QRectF(x, 0, cellW, h), c);
    }
}
