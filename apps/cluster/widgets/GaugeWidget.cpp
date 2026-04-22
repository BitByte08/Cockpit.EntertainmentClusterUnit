#include "GaugeWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QtMath>

GaugeWidget::GaugeWidget(QWidget *parent) : QWidget(parent) {
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        if (current_value_ == target_value_) return;
        int diff = target_value_ - current_value_;
        current_value_ += diff / 4 + (diff > 0 ? 1 : -1);
        if ((diff > 0 && current_value_ > target_value_) ||
            (diff < 0 && current_value_ < target_value_))
            current_value_ = target_value_;
        update();
    });
    timer->start(16);
}

void GaugeWidget::setValue(int value) {
    target_value_ = qBound(min_value_, value, max_value_);
}

void GaugeWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height());
    drawBackground(p, side);
    drawScale     (p, side);
    drawNeedle    (p, side);
    drawCenter    (p, side);
    drawDigital   (p, side);
}

// ── 배경 ──────────────────────────────────────────────────────────────────────

void GaugeWidget::drawBackground(QPainter &p, int side) {
    p.save();
    p.translate(width() / 2.0, height() / 2.0);
    p.scale(side / 300.0, side / 300.0);

    // 외곽 링
    QRadialGradient outerGrad(0, 0, 148);
    outerGrad.setColorAt(0.0,  QColor(0x12, 0x12, 0x1A));
    outerGrad.setColorAt(0.88, QColor(0x1A, 0x1A, 0x26));
    outerGrad.setColorAt(1.0,  QColor(0x40, 0x40, 0x58));
    p.setBrush(outerGrad);
    p.setPen(QPen(QColor(0x50, 0x50, 0x70), 1.5));
    p.drawEllipse(QPoint(0,0), 148, 148);

    // 내부 다크 그라디언트
    QRadialGradient innerGrad(0, -20, 130);
    innerGrad.setColorAt(0.0, QColor(0x0C, 0x0C, 0x14));
    innerGrad.setColorAt(1.0, QColor(0x08, 0x08, 0x10));
    p.setBrush(innerGrad);
    p.setPen(QPen(QColor(0x28, 0x28, 0x3A), 1));
    p.drawEllipse(QPoint(0,0), 135, 135);

    // 레드존 아크
    if (red_zone_start_ > min_value_ && red_zone_start_ < max_value_) {
        double redRatio = static_cast<double>(red_zone_start_ - min_value_) /
                          (max_value_ - min_value_);
        double startAngle = 225.0 - 270.0 * redRatio;
        double span       = 270.0 * (1.0 - redRatio);

        // 글로우
        p.setPen(QPen(QColor(255, 30, 30, 55), 10, Qt::SolidLine, Qt::FlatCap));
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRect(-138, -138, 276, 276),
                  static_cast<int>((90.0 - startAngle) * 16),
                  static_cast<int>(-span * 16));

        // 실선
        p.setPen(QPen(QColor(0xFF, 0x22, 0x22), 5, Qt::SolidLine, Qt::FlatCap));
        p.drawArc(QRect(-138, -138, 276, 276),
                  static_cast<int>((90.0 - startAngle) * 16),
                  static_cast<int>(-span * 16));
    }

    p.restore();
}

// ── 눈금 ──────────────────────────────────────────────────────────────────────

void GaugeWidget::drawScale(QPainter &p, int side) {
    p.save();
    p.translate(width() / 2.0, height() / 2.0);
    p.scale(side / 300.0, side / 300.0);

    const double startAngle = 225.0;
    const double spanAngle  = 270.0;
    const int    majTicks   = major_ticks_;
    const int    minTicks   = 4;

    for (int i = 0; i <= majTicks; ++i) {
        double angle = startAngle - spanAngle * i / majTicks;
        double rad   = qDegreesToRadians(angle);
        int    val   = min_value_ + (max_value_ - min_value_) * i / majTicks;
        bool   red   = (val >= red_zone_start_);

        QColor tickCol = red ? QColor(0xFF, 0x44, 0x44) : QColor(0xCC, 0xCC, 0xDD);
        p.setPen(QPen(tickCol, red ? 3 : 2.5));
        p.drawLine(QPointF(108 * qCos(rad), -108 * qSin(rad)),
                   QPointF(128 * qCos(rad), -128 * qSin(rad)));

        // 숫자 라벨
        QString text = QString::number(val / 1000);
        QFont f;
        f.setPointSize(red ? 12 : 11);
        f.setBold(true);
        p.setFont(f);
        p.setPen(red ? QColor(0xFF, 0x77, 0x77) : QColor(0xCC, 0xCC, 0xDD));
        int tx = static_cast<int>(88 * qCos(rad));
        int ty = static_cast<int>(-88 * qSin(rad));
        p.drawText(QRect(tx - 18, ty - 12, 36, 24), Qt::AlignCenter, text);
    }

    // 보조 눈금
    p.setPen(QPen(QColor(0x44, 0x44, 0x60), 1));
    for (int i = 0; i < majTicks; ++i) {
        for (int j = 1; j < minTicks; ++j) {
            double angle = startAngle - spanAngle * (i + j / static_cast<double>(minTicks)) / majTicks;
            double rad   = qDegreesToRadians(angle);
            p.drawLine(QPointF(120 * qCos(rad), -120 * qSin(rad)),
                       QPointF(128 * qCos(rad), -128 * qSin(rad)));
        }
    }

    p.restore();
}

// ── 바늘 ──────────────────────────────────────────────────────────────────────

void GaugeWidget::drawNeedle(QPainter &p, int side) {
    p.save();
    p.translate(width() / 2.0, height() / 2.0);
    p.scale(side / 300.0, side / 300.0);

    double ratio = (max_value_ > min_value_) ?
        qBound(0.0, static_cast<double>(current_value_ - min_value_) / (max_value_ - min_value_), 1.0) : 0.0;
    double angle = 225.0 - 270.0 * ratio;

    p.rotate(-angle + 90.0);

    // 그림자
    p.save();
    p.translate(3, 5);
    p.setOpacity(0.3);
    QPainterPath shadow;
    shadow.moveTo(0, 0);
    shadow.lineTo(-3, -10);
    shadow.lineTo(-1, -118);
    shadow.lineTo(0, -128);
    shadow.lineTo(1, -118);
    shadow.lineTo(3, -10);
    shadow.closeSubpath();
    p.fillPath(shadow, Qt::black);
    p.setOpacity(1.0);
    p.restore();

    // 바늘 본체
    QPainterPath needle;
    needle.moveTo(0, 0);
    needle.lineTo(-3, -10);
    needle.lineTo(-1, -118);
    needle.lineTo(0, -128);
    needle.lineTo(1, -118);
    needle.lineTo(3, -10);
    needle.closeSubpath();

    QColor needleColor;
    if (ratio < 0.6)       needleColor = QColor(0x00, 0xD8, 0xFF);
    else if (ratio < 0.82) needleColor = QColor(0xFF, 0xC0, 0x00);
    else                   needleColor = QColor(0xFF, 0x33, 0x33);

    QLinearGradient grad(0, -128, 0, 0);
    grad.setColorAt(0.0, needleColor.lighter(160));
    grad.setColorAt(0.5, needleColor);
    grad.setColorAt(1.0, needleColor.darker(130));

    p.setBrush(grad);
    p.setPen(QPen(needleColor.darker(160), 0.5));
    p.drawPath(needle);

    p.restore();
}

// ── 중심 허브 ────────────────────────────────────────────────────────────────

void GaugeWidget::drawCenter(QPainter &p, int side) {
    p.save();
    p.translate(width() / 2.0, height() / 2.0);
    p.scale(side / 300.0, side / 300.0);

    // 외부 링
    QRadialGradient hubGrad(0, -2, 14);
    hubGrad.setColorAt(0.0, QColor(0xC0, 0xC0, 0xD0));
    hubGrad.setColorAt(0.5, QColor(0x60, 0x60, 0x80));
    hubGrad.setColorAt(1.0, QColor(0x28, 0x28, 0x38));
    p.setBrush(hubGrad);
    p.setPen(QPen(QColor(0x50, 0x50, 0x70), 1));
    p.drawEllipse(QPoint(0,0), 13, 13);

    // 중심 레드 닷
    p.setBrush(QColor(0xFF, 0x22, 0x22));
    p.setPen(Qt::NoPen);
    p.drawEllipse(QPoint(0,0), 5, 5);

    p.restore();
}

// ── 디지털 값 표시 ───────────────────────────────────────────────────────────

void GaugeWidget::drawDigital(QPainter &p, int side) {
    p.save();
    p.translate(width() / 2.0, height() / 2.0);
    p.scale(side / 300.0, side / 300.0);

    // 라벨 (위쪽)
    {
        QFont f;
        f.setPointSize(10);
        f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 2);
        p.setFont(f);
        p.setPen(QColor(0x00, 0xCC, 0xBB));
        p.drawText(QRect(-70, -62, 140, 18), Qt::AlignCenter, label_);
        p.setPen(QPen(QColor(0x00, 0xCC, 0xBB, 180), 1));
        p.drawLine(-30, -46, 30, -46);
    }

    // 값 (아래쪽 중앙)
    {
        QFont f;
        f.setPointSize(32);
        f.setBold(true);
        p.setFont(f);
        p.setPen(Qt::white);
        p.drawText(QRect(-70, 30, 140, 48), Qt::AlignCenter, QString::number(current_value_));
    }

    // 단위
    {
        QFont f;
        f.setPointSize(10);
        p.setFont(f);
        p.setPen(QColor(0x77, 0x77, 0x99));
        p.drawText(QRect(-50, 76, 100, 16), Qt::AlignCenter, unit_);
    }

    p.restore();
}
