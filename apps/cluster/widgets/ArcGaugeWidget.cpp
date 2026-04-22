#include "ArcGaugeWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QtMath>
#include <QConicalGradient>

ArcGaugeWidget::ArcGaugeWidget(QWidget *parent) : QWidget(parent) {
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        if (current_ == target_) return;
        int diff = target_ - current_;
        current_ += diff / 4 + (diff > 0 ? 1 : -1);
        if ((diff > 0 && current_ > target_) || (diff < 0 && current_ < target_))
            current_ = target_;
        update();
    });
    timer->start(16);
}

void ArcGaugeWidget::setValue(int value) {
    target_ = qBound(min_, value, max_);
}

// 비율(0~1)에 따른 아크 색상
QColor ArcGaugeWidget::arcColor(double ratio) const {
    if (warn_threshold_ >= 0) {
        // 수온 모드 (high_is_warn_=true): 낮을수록 파랑, 높을수록 빨강
        if (high_is_warn_) {
            double wRatio = static_cast<double>(warn_threshold_ - min_) / (max_ - min_);
            if (ratio < wRatio * 0.7)        return QColor(0x00, 0xAA, 0xFF); // 파랑(냉각)
            if (ratio < wRatio)              return QColor(0x00, 0xCC, 0x55); // 녹색(정상)
            if (ratio < wRatio + 0.1)        return QColor(0xFF, 0xB3, 0x00); // 황색(주의)
            return QColor(0xFF, 0x33, 0x33);                                   // 적색(과열)
        }
        // 연료 모드 (high_is_warn_=false): 높을수록 좋음
        double wRatio = static_cast<double>(warn_threshold_ - min_) / (max_ - min_);
        if (ratio > wRatio * 2.0)  return QColor(0x00, 0xCC, 0x55); // 녹색(충분)
        if (ratio > wRatio)        return QColor(0xFF, 0xB3, 0x00); // 황색(주의)
        return QColor(0xFF, 0x33, 0x33);                             // 적색(부족)
    }
    // 기본: 녹→황→적
    if (ratio < 0.6) return QColor(0x00, 0xCC, 0x55);
    if (ratio < 0.8) return QColor(0xFF, 0xB3, 0x00);
    return QColor(0xFF, 0x33, 0x33);
}

void ArcGaugeWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width(), h = height();
    // 아크 중심: 가로 중앙, 세로 약 60% 지점 (반원이 위로 열리도록)
    const double cx = w * 0.5;
    const double cy = h * 0.62;
    const int radius = static_cast<int>(qMin(cx, cy) * 0.88);
    const int arcThick = qMax(10, radius / 9);

    QRectF arcRect(cx - radius, cy - radius, radius * 2.0, radius * 2.0);

    // ── 아크 각도: 225° 시작, -270° 스팬 (Qt CCW positive, 3시 방향 = 0°) ──
    // 225° = 왼쪽 하단(7시 방향), -270° 스팬 = 시계방향 270° → 오른쪽 하단(5시 방향)
    const double startAngle = 225.0;
    const double spanAngle  = -270.0;

    // 배경 아크
    p.setPen(QPen(QColor(0x1A, 0x1A, 0x2A), arcThick, Qt::SolidLine, Qt::FlatCap));
    p.setBrush(Qt::NoBrush);
    p.drawArc(arcRect, static_cast<int>(startAngle * 16), static_cast<int>(spanAngle * 16));

    // 값 아크
    double ratio = (max_ > min_) ?
        qBound(0.0, static_cast<double>(current_ - min_) / (max_ - min_), 1.0) : 0.0;

    if (ratio > 0.0) {
        QColor ac = arcColor(ratio);
        // 글로우 효과 (넓고 투명한 선)
        p.setPen(QPen(QColor(ac.red(), ac.green(), ac.blue(), 60),
                      arcThick + 6, Qt::SolidLine, Qt::FlatCap));
        p.drawArc(arcRect,
                  static_cast<int>(startAngle * 16),
                  static_cast<int>(spanAngle * ratio * 16));

        p.setPen(QPen(ac, arcThick, Qt::SolidLine, Qt::FlatCap));
        p.drawArc(arcRect,
                  static_cast<int>(startAngle * 16),
                  static_cast<int>(spanAngle * ratio * 16));
    }

    // 눈금 틱 (최소/최대/중간)
    {
        p.setPen(QPen(QColor(0x44, 0x44, 0x66), 1));
        int ticks = 5;
        for (int i = 0; i <= ticks; i++) {
            double frac = static_cast<double>(i) / ticks;
            double angleDeg = startAngle + spanAngle * frac;
            double rad = qDegreesToRadians(angleDeg);
            double innerR = radius + arcThick / 2 + 3;
            double outerR = innerR + (i == 0 || i == ticks ? 6 : 4);
            p.drawLine(QPointF(cx + innerR * qCos(rad), cy - innerR * qSin(rad)),
                       QPointF(cx + outerR * qCos(rad), cy - outerR * qSin(rad)));
        }
    }

    // ── 라벨 (상단) ──────────────────────────────────────────────────────────
    {
        QFont f;
        f.setPointSize(9);
        f.setBold(true);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 2);
        p.setFont(f);
        p.setPen(QColor(0x00, 0xCC, 0xBB));
        int labelY = static_cast<int>(cy - radius * 0.78);
        p.drawText(QRect(0, labelY, w, 16), Qt::AlignCenter, label_);
    }

    // ── 값 텍스트 (중앙) ─────────────────────────────────────────────────────
    {
        QFont f;
        f.setPointSize(28);
        f.setBold(true);
        p.setFont(f);

        // 경고 상태 색상
        bool warn = (warn_threshold_ >= 0) &&
                    (high_is_warn_ ? current_ >= warn_threshold_
                                   : current_ <= warn_threshold_);
        p.setPen(warn ? QColor(0xFF, 0x33, 0x33) : Qt::white);
        p.drawText(QRect(0, static_cast<int>(cy - radius * 0.38), w, 40),
                   Qt::AlignCenter, QString::number(current_));
    }

    // ── 단위 텍스트 ───────────────────────────────────────────────────────────
    if (!unit_.isEmpty()) {
        QFont f;
        f.setPointSize(10);
        p.setFont(f);
        p.setPen(QColor(0x88, 0x88, 0xAA));
        p.drawText(QRect(0, static_cast<int>(cy + 8), w, 16), Qt::AlignCenter, unit_);
    }
}
