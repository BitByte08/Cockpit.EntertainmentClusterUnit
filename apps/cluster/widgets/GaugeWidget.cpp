#include "GaugeWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QtMath>

GaugeWidget::GaugeWidget(QWidget *parent) : QWidget(parent) {
    setMinimumSize(200, 200);
    
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        if (current_value_ != target_value_) {
            int diff = target_value_ - current_value_;
            current_value_ += diff / 4 + (diff > 0 ? 1 : -1);
            
            if ((diff > 0 && current_value_ > target_value_) ||
                (diff < 0 && current_value_ < target_value_)) {
                current_value_ = target_value_;
            }
            update();
        }
    });
    timer->start(16);
}

void GaugeWidget::setValue(int value) {
    target_value_ = qBound(min_value_, value, max_value_);
}

QSize GaugeWidget::sizeHint() const {
    return QSize(320, 320);
}

QSize GaugeWidget::minimumSizeHint() const {
    return QSize(160, 160);
}

void GaugeWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    drawBackground(painter);
    drawScale(painter);
    drawNeedle(painter);
    drawValue(painter);
    drawLabel(painter);
}

void GaugeWidget::drawBackground(QPainter &painter) {
    int side = qMin(width(), height());
    painter.save();
    painter.translate(width() / 2, height() / 2);
    painter.scale(side / 320.0, side / 320.0);

    // 외곽 링 - 검은 테두리
    QRadialGradient outerGrad(0, 0, 160);
    outerGrad.setColorAt(0.0, QColor(20, 20, 20));
    outerGrad.setColorAt(0.95, QColor(40, 40, 40));
    outerGrad.setColorAt(1.0, QColor(80, 80, 80));
    
    painter.setBrush(outerGrad);
    painter.setPen(QPen(QColor(100, 100, 100), 2));
    painter.drawEllipse(QPoint(0, 0), 158, 158);
    
    // 내측 그라디언트 - 스포츠 느낌의 다크 그레이
    QRadialGradient innerGrad(0, 0, 140);
    innerGrad.setColorAt(0.0, QColor(15, 15, 15));
    innerGrad.setColorAt(1.0, QColor(25, 25, 25));
    
    painter.setBrush(innerGrad);
    painter.setPen(QPen(QColor(60, 60, 60), 1));
    painter.drawEllipse(QPoint(0, 0), 145, 145);
    
    // Red Zone 아크 (마지막 10%)
    if (red_zone_start_ > min_value_) {
        int redStart = red_zone_start_;
        double redRatio = (double)(redStart - min_value_) / (max_value_ - min_value_);
        double startAngle = 225.0 - (270.0 * redRatio);
        double span = 270.0 * (1.0 - redRatio);
        
        QPen redPen(QColor(255, 50, 50), 6, Qt::SolidLine, Qt::FlatCap);
        painter.setPen(redPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawArc(QRect(-150, -150, 300, 300), 
                       (90 - startAngle) * 16, span * -16);
    }

    painter.restore();
}

void GaugeWidget::drawScale(QPainter &painter) {
    int side = qMin(width(), height());
    painter.save();
    painter.translate(width() / 2, height() / 2);
    painter.scale(side / 320.0, side / 320.0);
    
    const double startAngle = 225.0;
    const double spanAngle = 270.0;
    const int majorTicks = major_ticks_;
    const int minorTicks = 4;
    
    // 메인 눈금
    for (int i = 0; i <= majorTicks; ++i) {
        double angle = startAngle - (spanAngle * i / majorTicks);
        double rad = qDegreesToRadians(angle);
        
        int value = min_value_ + (max_value_ - min_value_) * i / majorTicks;
        bool isRedZone = value >= red_zone_start_;
        
        QColor tickColor = isRedZone ? QColor(255, 50, 50) : QColor(200, 200, 200);
        painter.setPen(QPen(tickColor, isRedZone ? 4 : 3));
        
        int x1 = 115 * qCos(rad);
        int y1 = -115 * qSin(rad);
        int x2 = 135 * qCos(rad);
        int y2 = -135 * qSin(rad);
        painter.drawLine(x1, y1, x2, y2);
        
        // 숫자
        if (i % 2 == 0 || isRedZone) {
            QString text = QString::number(value / 1000);
            int tx = 95 * qCos(rad);
            int ty = -95 * qSin(rad);
            
            painter.save();
            painter.setPen(isRedZone ? QColor(255, 80, 80) : QColor(220, 220, 220));
            QFont font = painter.font();
            font.setPointSize(isRedZone ? 14 : 12);
            font.setBold(true);
            painter.setFont(font);
            
            QRect textRect(tx - 25, ty - 12, 50, 24);
            painter.drawText(textRect, Qt::AlignCenter, text);
            painter.restore();
        }
    }
    
    // 보조 눈금
    painter.setPen(QPen(QColor(80, 80, 80), 1));
    for (int i = 0; i < majorTicks; ++i) {
        for (int j = 1; j < minorTicks; ++j) {
            double angle = startAngle - (spanAngle * (i + j / (double)minorTicks) / majorTicks);
            double rad = qDegreesToRadians(angle);
            
            int x1 = 125 * qCos(rad);
            int y1 = -125 * qSin(rad);
            int x2 = 135 * qCos(rad);
            int y2 = -135 * qSin(rad);
            
            painter.drawLine(x1, y1, x2, y2);
        }
    }
    
    painter.restore();
}

void GaugeWidget::drawNeedle(QPainter &painter) {
    int side = qMin(width(), height());
    painter.save();
    painter.translate(width() / 2, height() / 2);
    painter.scale(side / 320.0, side / 320.0);
    
    double ratio = (double)(current_value_ - min_value_) / (max_value_ - min_value_);
    double angle = 225.0 - (270.0 * ratio);
    
    painter.rotate(-angle + 90);
    
    // 바늘 - 날카로운 스포츠 디자인
    QPainterPath needlePath;
    needlePath.moveTo(0, 0);
    needlePath.lineTo(-4, -8);
    needlePath.lineTo(-1, -125);
    needlePath.lineTo(0, -135);
    needlePath.lineTo(1, -125);
    needlePath.lineTo(4, -8);
    needlePath.closeSubpath();
    
    // 바늘 색상
    QColor needleColor;
    if (ratio < 0.6) {
        needleColor = QColor(0, 200, 255);  // 청색
    } else if (ratio < 0.8) {
        needleColor = QColor(255, 200, 0);  // 황색
    } else {
        needleColor = QColor(255, 50, 50);   // 적색
    }
    
    QLinearGradient needleGrad(0, -135, 0, 0);
    needleGrad.setColorAt(0.0, needleColor.lighter(150));
    needleGrad.setColorAt(0.5, needleColor);
    needleGrad.setColorAt(1.0, needleColor.darker(120));
    
    painter.setBrush(needleGrad);
    painter.setPen(QPen(needleColor.darker(150), 1));
    painter.drawPath(needlePath);
    
    // 중심 허브
    QRadialGradient hubGrad(0, 0, 15);
    hubGrad.setColorAt(0.0, QColor(200, 200, 200));
    hubGrad.setColorAt(0.6, QColor(80, 80, 80));
    hubGrad.setColorAt(1.0, QColor(40, 40, 40));
    
    painter.setBrush(hubGrad);
    painter.setPen(QPen(QColor(60, 60, 60), 2));
    painter.drawEllipse(QPoint(0, 0), 12, 12);
    
    // 허브 중심 레드 닷
    painter.setBrush(QColor(255, 50, 50));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPoint(0, 0), 5, 5);
    
    painter.restore();
}

void GaugeWidget::drawValue(QPainter &painter) {
    int side = qMin(width(), height());
    painter.save();
    painter.translate(width() / 2, height() / 2);
    painter.scale(side / 320.0, side / 320.0);
    
    // 현재 값 - 디지털 스타일
    painter.setPen(QColor(255, 255, 255));
    QFont font = painter.font();
    font.setPointSize(42);
    font.setBold(true);
    font.setFamily("Arial Black");
    painter.setFont(font);
    
    QString valueText = QString::number(current_value_);
    QRect valueRect(-80, 45, 160, 50);
    painter.drawText(valueRect, Qt::AlignCenter, valueText);
    
    // 단위
    font.setPointSize(14);
    font.setBold(false);
    painter.setFont(font);
    painter.setPen(QColor(150, 150, 150));
    QRect unitRect(-60, 90, 120, 20);
    painter.drawText(unitRect, Qt::AlignCenter, unit_);
    
    painter.restore();
}

void GaugeWidget::drawLabel(QPainter &painter) {
    int side = qMin(width(), height());
    painter.save();
    painter.translate(width() / 2, height() / 2);
    painter.scale(side / 320.0, side / 320.0);
    
    // 라벨
    painter.setPen(QColor(0, 200, 255));
    QFont font = painter.font();
    font.setPointSize(16);
    font.setBold(true);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 3);
    painter.setFont(font);
    
    QRect labelRect(-100, -35, 200, 30);
    painter.drawText(labelRect, Qt::AlignCenter, label_);
    
    // 언더라인
    painter.setPen(QPen(QColor(0, 200, 255), 2));
    painter.drawLine(-40, -8, 40, -8);
    
    painter.restore();
}
