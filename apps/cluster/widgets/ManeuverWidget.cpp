#include "ManeuverWidget.hpp"
#include <QPainter>
#include <QPainterPath>

ManeuverWidget::ManeuverWidget(QWidget *parent) : QWidget(parent) {
    setFixedSize(80, 80);
    hide();
}

void ManeuverWidget::onManeuverChanged(int type, int distMeters) {
    type_ = static_cast<Maneuver>(type);
    dist_ = distMeters;
    const bool visible = (type_ != Maneuver::None);
    setVisible(visible);
    if (visible) update();
}

void ManeuverWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 배경
    p.setBrush(QColor(0, 0, 0, 220));
    p.setPen(QPen(QColor(0x3c, 0x3c, 0x3c), 1));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);

    const int W = width(), H = height();
    const int arrowAreaH = 50;

    // ── 화살표 ──────────────────────────────────────────────────────────────────
    p.save();
    p.translate(W / 2.0, arrowAreaH / 2.0 + 4);
    p.setPen(Qt::NoPen);

    if (type_ == Maneuver::Arrived) {
        // 체크 원
        p.setBrush(QColor(0x0f, 0xa3, 0x36));
        p.drawEllipse(QPoint(0, 0), 17, 17);
        p.setPen(QPen(Qt::white, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        QPainterPath chk;
        chk.moveTo(-7, 0); chk.lineTo(-2, 6); chk.lineTo(8, -6);
        p.drawPath(chk);
    } else {
        QPainterPath arrow;
        if (type_ == Maneuver::Straight) {
            arrow.moveTo( 0, -18);
            arrow.lineTo(-7,  -7);
            arrow.lineTo(-3,  -7);
            arrow.lineTo(-3,  14);
            arrow.lineTo( 3,  14);
            arrow.lineTo( 3,  -7);
            arrow.lineTo( 7,  -7);
            arrow.closeSubpath();
        } else if (type_ == Maneuver::TurnRight) {
            arrow.moveTo( 18,   0);
            arrow.lineTo(  7,  -8);
            arrow.lineTo(  7,  -3);
            arrow.lineTo(-11,  -3);
            arrow.lineTo(-11, -11);
            arrow.lineTo(-18,   0);
            arrow.lineTo(-11,  11);
            arrow.lineTo(-11,   3);
            arrow.lineTo(  7,   3);
            arrow.lineTo(  7,   8);
            arrow.closeSubpath();
        } else {
            // TurnLeft — mirror of TurnRight
            p.scale(-1, 1);
            arrow.moveTo( 18,   0);
            arrow.lineTo(  7,  -8);
            arrow.lineTo(  7,  -3);
            arrow.lineTo(-11,  -3);
            arrow.lineTo(-11, -11);
            arrow.lineTo(-18,   0);
            arrow.lineTo(-11,  11);
            arrow.lineTo(-11,   3);
            arrow.lineTo(  7,   3);
            arrow.lineTo(  7,   8);
            arrow.closeSubpath();
        }
        p.setBrush(QColor(0x1c, 0x69, 0xd4));
        p.drawPath(arrow);
    }
    p.restore();

    // ── 거리 텍스트 ──────────────────────────────────────────────────────────────
    if (dist_ > 0 && type_ != Maneuver::Arrived) {
        QString txt = dist_ >= 1000
            ? QStringLiteral("%1 km").arg(dist_ / 1000.0, 0, 'f', 1)
            : QStringLiteral("%1 m").arg(dist_);
        QFont f; f.setPointSize(7); f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(0xff, 0xff, 0xff));
        p.drawText(QRect(0, arrowAreaH, W, H - arrowAreaH),
                   Qt::AlignHCenter | Qt::AlignVCenter, txt);
    }
}
