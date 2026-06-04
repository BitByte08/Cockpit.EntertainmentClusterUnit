#include "IndicatorWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QPolygon>
#include <QApplication>

QTimer *IndicatorWidget::s_shared_timer_{nullptr};
bool    IndicatorWidget::s_blink_state_{false};
int     IndicatorWidget::s_ref_count_{0};

void IndicatorWidget::ensureSharedTimer() {
    if (!s_shared_timer_) {
        s_shared_timer_ = new QTimer;
        s_shared_timer_->setInterval(500);
    }
}

IndicatorWidget::IndicatorWidget(IndicatorIcon icon,
                                  const QString &label,
                                  const QColor  &activeColor,
                                  QWidget       *parent)
    : QWidget(parent), icon_(icon), label_(label), active_color_(activeColor)
{
    ensureSharedTimer();
    setAttribute(Qt::WA_TranslucentBackground, false);
}

void IndicatorWidget::setActive(bool active) {
    if (!active && blinking_) {
        s_ref_count_--;
        if (s_ref_count_ <= 0) {
            s_shared_timer_->stop();
            s_shared_timer_->disconnect();
            s_ref_count_ = 0;
        }
        blinking_ = false;
    }
    active_ = active;
    update();
}

void IndicatorWidget::startBlink() {
    active_ = true;
    if (!blinking_) {
        blinking_ = true;
        s_ref_count_++;
        if (s_ref_count_ == 1) {
            s_blink_state_ = false;
            QObject::connect(s_shared_timer_, &QTimer::timeout, []() {
                s_blink_state_ = !s_blink_state_;
                for (auto *w : QApplication::allWidgets()) {
                    auto *ind = qobject_cast<IndicatorWidget *>(w);
                    if (ind) ind->update();
                }
            });
            s_shared_timer_->start();
        }
    }
    update();
}

void IndicatorWidget::stopBlink() {
    if (blinking_) {
        s_ref_count_--;
        if (s_ref_count_ <= 0) {
            s_shared_timer_->stop();
            s_shared_timer_->disconnect();
            s_ref_count_ = 0;
        }
        blinking_ = false;
    }
    update();
}

void IndicatorWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0x00, 0x00, 0x00));
    bool lit = active_ && !(blinking_ && s_blink_state_);
    QColor iconColor = lit ? active_color_ : kDimColor;
    constexpr int kLabelH = 13;
    int iw = width(), ih = height() - kLabelH;
    drawIcon(p, iconColor, iw, ih);
    QFont f; f.setPointSize(6); f.setBold(true);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    p.setFont(f);
    p.setPen(lit ? iconColor.darker(115) : QColor(0x40, 0x40, 0x55));
    p.drawText(QRect(0, ih, width(), kLabelH), Qt::AlignCenter, label_);
}

void IndicatorWidget::drawIcon(QPainter &p, const QColor &c, int iw, int ih) {
    switch (icon_) {
    case IndicatorIcon::TurnLeft:    drawTurnArrow(p, true,  c, iw, ih); break;
    case IndicatorIcon::TurnRight:   drawTurnArrow(p, false, c, iw, ih); break;
    case IndicatorIcon::HighBeam:
    case IndicatorIcon::LowBeam:     drawHighBeam(p, c, iw, ih);         break;
    case IndicatorIcon::CheckEngine: drawCheckEngine(p, c, iw, ih);      break;
    case IndicatorIcon::OilPressure: drawOilPressure(p, c, iw, ih);      break;
    case IndicatorIcon::ABS:         drawTextBox(p, "ABS", c, iw, ih);   break;
    case IndicatorIcon::TCS:         drawTextBox(p, "TCS", c, iw, ih);   break;
    case IndicatorIcon::FuelWarn:    drawFuelWarn(p, c, iw, ih);         break;
    case IndicatorIcon::Battery:     drawBattery(p, c, iw, ih);          break;
    }
}

void IndicatorWidget::drawTurnArrow(QPainter &p, bool left, const QColor &c, int iw, int ih) {
    const int aw = 40, ah = 24, ox = (iw-aw)/2, oy = (ih-ah)/2;
    QPolygon arrow;
    if (left) {
        arrow << QPoint(ox,oy+ah/2) << QPoint(ox+aw/2,oy)
              << QPoint(ox+aw/2,oy+ah/4) << QPoint(ox+aw,oy+ah/4)
              << QPoint(ox+aw,oy+3*ah/4) << QPoint(ox+aw/2,oy+3*ah/4)
              << QPoint(ox+aw/2,oy+ah);
    } else {
        arrow << QPoint(ox+aw,oy+ah/2) << QPoint(ox+aw/2,oy)
              << QPoint(ox+aw/2,oy+ah/4) << QPoint(ox,oy+ah/4)
              << QPoint(ox,oy+3*ah/4) << QPoint(ox+aw/2,oy+3*ah/4)
              << QPoint(ox+aw/2,oy+ah);
    }
    p.setBrush(c); p.setPen(Qt::NoPen); p.drawPolygon(arrow);
}

void IndicatorWidget::drawHighBeam(QPainter &p, const QColor &c, int iw, int ih) {
    const int cx=iw/2-4, cy=ih/2, r=9;
    p.setBrush(c); p.setPen(Qt::NoPen);
    QPainterPath arc; arc.moveTo(cx,cy-r);
    arc.arcTo(cx-r,cy-r,r*2,r*2,90,180); arc.closeSubpath(); p.fillPath(arc,c);
    QPen pen(c,2.5f,Qt::SolidLine,Qt::RoundCap); p.setPen(pen);
    int beamX=cx+r+2,right=iw-4,bo[]={-7,-3,2,6};
    for(int dy:bo) p.drawLine(beamX,cy+dy,right,cy+dy);
}

void IndicatorWidget::drawCheckEngine(QPainter &p, const QColor &c, int iw, int ih) {
    const int bx=(iw-34)/2, by=ih/2-3;
    QPen pen(c,2,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin); p.setPen(pen); p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(bx,by,34,14,2,2);
    for(int i=0;i<2;i++) p.drawRect(bx+7+i*14,by-8,8,8);
    p.drawLine(bx+34,by+7,bx+38,by+7); p.drawLine(bx+38,by+7,bx+38,by+11);
}

void IndicatorWidget::drawOilPressure(QPainter &p, const QColor &c, int iw, int ih) {
    const int ox=(iw-30)/2, oy=ih/2-10;
    p.setBrush(c); p.setPen(Qt::NoPen);
    p.drawRoundedRect(ox,oy+8,18,12,2,2); p.drawRect(ox+4,oy,8,8);
    QPolygon spout; spout << QPoint(ox+18,oy+4) << QPoint(ox+28,oy+2)
          << QPoint(ox+28,oy+8) << QPoint(ox+18,oy+8); p.drawPolygon(spout);
    QPainterPath drop; int dx=ox+8,dy=oy+22;
    drop.moveTo(dx,dy+6); drop.cubicTo(dx-4,dy+4,dx-4,dy,dx,dy-1);
    drop.cubicTo(dx+4,dy,dx+4,dy+4,dx,dy+6); p.fillPath(drop,c);
}

void IndicatorWidget::drawTextBox(QPainter &p, const QString &text, const QColor &c, int iw, int ih) {
    const int bw=36,bh=20,bx=(iw-bw)/2,by=(ih-bh)/2;
    p.setBrush(Qt::NoBrush); p.setPen(QPen(c,2)); p.drawRoundedRect(bx,by,bw,bh,3,3);
    QFont f; f.setPointSize(10); f.setBold(true); p.setFont(f); p.setPen(c);
    p.drawText(QRect(bx,by,bw,bh),Qt::AlignCenter,text);
}

void IndicatorWidget::drawFuelWarn(QPainter &p, const QColor &c, int iw, int ih) {
    const int ox=(iw-28)/2, oy=ih/2-10;
    p.setBrush(c); p.setPen(Qt::NoPen); p.drawRoundedRect(ox,oy,16,20,2,2);
    QPen pen(c,2.5f,Qt::SolidLine,Qt::RoundCap); p.setPen(pen);
    p.drawLine(ox+16,oy+4,ox+24,oy+4); p.drawLine(ox+24,oy+4,ox+24,oy+12);
    p.drawLine(ox+22,oy+12,ox+26,oy+12);
    p.setPen(QPen(QColor(0x08,0x08,0x12),2));
    p.drawLine(ox+4,oy+10,ox+12,oy+10); p.drawLine(ox+8,oy+6,ox+8,oy+14);
}

void IndicatorWidget::drawBattery(QPainter &p, const QColor &c, int iw, int ih) {
    const int bw=32,bh=16,bx=(iw-bw)/2,by=(ih-bh)/2;
    p.setBrush(Qt::NoBrush); p.setPen(QPen(c,2)); p.drawRoundedRect(bx,by,bw-4,bh,2,2);
    p.drawRect(bx+bw-4,by+4,4,bh-8); p.setPen(QPen(c,1.5f));
    int mx=bx+(bw-4)/2,my=by+bh/2;
    p.drawLine(mx-8,my,mx-4,my); p.drawLine(mx+4,my,mx+8,my);
    p.drawLine(mx+6,my-2,mx+6,my+2);
}
