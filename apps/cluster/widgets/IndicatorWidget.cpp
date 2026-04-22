#include "IndicatorWidget.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QPolygon>

IndicatorWidget::IndicatorWidget(IndicatorIcon icon,
                                  const QString &label,
                                  const QColor  &activeColor,
                                  QWidget       *parent)
    : QWidget(parent),
      icon_(icon),
      label_(label),
      active_color_(activeColor)
{
    blink_timer_ = new QTimer(this);
    blink_timer_->setInterval(500);
    connect(blink_timer_, &QTimer::timeout, this, &IndicatorWidget::onBlink);
    setAttribute(Qt::WA_TranslucentBackground, false);
}

// ── 공개 제어 메서드 ──────────────────────────────────────────────────────────

void IndicatorWidget::setActive(bool active) {
    if (!active) {
        blink_timer_->stop();
        blinking_ = false;
        blink_state_ = false;
    }
    active_ = active;
    update();
}

void IndicatorWidget::startBlink() {
    active_  = true;
    blinking_ = true;
    if (!blink_timer_->isActive()) {
        blink_state_ = false;
        blink_timer_->start();
    }
    update();
}

void IndicatorWidget::stopBlink() {
    blink_timer_->stop();
    blinking_    = false;
    blink_state_ = false;
    update();
}

void IndicatorWidget::onBlink() {
    blink_state_ = !blink_state_;
    update();
}

// ── 페인트 이벤트 ─────────────────────────────────────────────────────────────

void IndicatorWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 배경: 투명 (부모 배경 그대로)
    p.fillRect(rect(), QColor(0x0A, 0x0A, 0x10));

    // 현재 아이콘 색상 결정
    bool lit = active_ && !(blinking_ && blink_state_);
    QColor iconColor = lit ? active_color_ : kDimColor;

    // 아이콘 영역: 위쪽 80%
    constexpr int kLabelH = 13;
    int iw = width();
    int ih = height() - kLabelH;

    drawIcon(p, iconColor, iw, ih);

    // 라벨 텍스트
    QFont f;
    f.setPointSize(6);
    f.setBold(true);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1);
    p.setFont(f);
    p.setPen(lit ? iconColor.darker(115) : QColor(0x40, 0x40, 0x55));
    p.drawText(QRect(0, ih, width(), kLabelH), Qt::AlignCenter, label_);
}

// ── 아이콘 디스패처 ───────────────────────────────────────────────────────────

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

// ── 개별 아이콘 그리기 ────────────────────────────────────────────────────────

void IndicatorWidget::drawTurnArrow(QPainter &p, bool left, const QColor &c, int iw, int ih) {
    // 화살표 폴리곤 (40×24 논리 좌표, 중앙 배치)
    const int aw = 40, ah = 24;
    const int ox = (iw - aw) / 2;
    const int oy = (ih - ah) / 2;

    QPolygon arrow;
    if (left) {
        // ◄ 형태
        arrow << QPoint(ox,         oy + ah/2)   // 팁 (왼쪽)
              << QPoint(ox + aw/2,  oy)           // 상단-좌측 날개
              << QPoint(ox + aw/2,  oy + ah/4)    // 내부 상단
              << QPoint(ox + aw,    oy + ah/4)    // 꼬리 우상
              << QPoint(ox + aw,    oy + 3*ah/4)  // 꼬리 우하
              << QPoint(ox + aw/2,  oy + 3*ah/4)  // 내부 하단
              << QPoint(ox + aw/2,  oy + ah);      // 하단-좌측 날개
    } else {
        // ► 형태
        arrow << QPoint(ox + aw,    oy + ah/2)
              << QPoint(ox + aw/2,  oy)
              << QPoint(ox + aw/2,  oy + ah/4)
              << QPoint(ox,         oy + ah/4)
              << QPoint(ox,         oy + 3*ah/4)
              << QPoint(ox + aw/2,  oy + 3*ah/4)
              << QPoint(ox + aw/2,  oy + ah);
    }
    p.setBrush(c);
    p.setPen(Qt::NoPen);
    p.drawPolygon(arrow);
}

void IndicatorWidget::drawHighBeam(QPainter &p, const QColor &c, int iw, int ih) {
    // 반원(헤드라이트) + 발산 광선 4개
    const int cx = iw / 2 - 4;
    const int cy = ih / 2;
    const int r  = 9;

    p.setBrush(c);
    p.setPen(Qt::NoPen);
    // 반원
    QPainterPath arc;
    arc.moveTo(cx, cy - r);
    arc.arcTo(cx - r, cy - r, r * 2, r * 2, 90, 180);
    arc.closeSubpath();
    p.fillPath(arc, c);

    // 광선
    QPen pen(c, 2.5f, Qt::SolidLine, Qt::RoundCap);
    p.setPen(pen);
    const int beamX = cx + r + 2;
    const int right = iw - 4;
    int beamOffsets[] = {-7, -3, 2, 6};
    for (int dy : beamOffsets) {
        p.drawLine(beamX, cy + dy, right, cy + dy);
    }
}

void IndicatorWidget::drawCheckEngine(QPainter &p, const QColor &c, int iw, int ih) {
    // 간략화된 엔진 블록 + 실린더 2개
    const int bx = (iw - 34) / 2;
    const int by = ih / 2 - 3;
    QPen pen(c, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    // 엔진 몸체
    p.drawRoundedRect(bx, by, 34, 14, 2, 2);
    // 실린더 2개 (위쪽)
    for (int i = 0; i < 2; i++) {
        int cx = bx + 7 + i * 14;
        p.drawRect(cx, by - 8, 8, 8);
    }
    // 배기관 (오른쪽)
    p.drawLine(bx + 34, by + 7, bx + 38, by + 7);
    p.drawLine(bx + 38, by + 7, bx + 38, by + 11);
}

void IndicatorWidget::drawOilPressure(QPainter &p, const QColor &c, int iw, int ih) {
    // 오일캔 실루엣 (단순화)
    const int ox = (iw - 30) / 2;
    const int oy = ih / 2 - 10;
    p.setBrush(c);
    p.setPen(Qt::NoPen);

    // 캔 몸체
    p.drawRoundedRect(ox, oy + 8, 18, 12, 2, 2);
    // 목 (윗부분)
    p.drawRect(ox + 4, oy, 8, 8);
    // 주둥이
    QPolygon spout;
    spout << QPoint(ox + 18, oy + 4)
          << QPoint(ox + 28, oy + 2)
          << QPoint(ox + 28, oy + 8)
          << QPoint(ox + 18, oy + 8);
    p.drawPolygon(spout);
    // 오일 방울
    p.setBrush(c);
    QPainterPath drop;
    int dx = ox + 8, dy = oy + 22;
    drop.moveTo(dx, dy + 6);
    drop.cubicTo(dx - 4, dy + 4, dx - 4, dy, dx, dy - 1);
    drop.cubicTo(dx + 4, dy, dx + 4, dy + 4, dx, dy + 6);
    p.fillPath(drop, c);
}

void IndicatorWidget::drawTextBox(QPainter &p, const QString &text, const QColor &c, int iw, int ih) {
    const int bw = 36, bh = 20;
    const int bx = (iw - bw) / 2;
    const int by = (ih - bh) / 2;

    // 외곽선 박스
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(c, 2));
    p.drawRoundedRect(bx, by, bw, bh, 3, 3);

    // 굵은 텍스트
    QFont f;
    f.setPointSize(10);
    f.setBold(true);
    p.setFont(f);
    p.setPen(c);
    p.drawText(QRect(bx, by, bw, bh), Qt::AlignCenter, text);
}

void IndicatorWidget::drawFuelWarn(QPainter &p, const QColor &c, int iw, int ih) {
    // 연료 펌프 실루엣
    const int ox = (iw - 28) / 2;
    const int oy = ih / 2 - 10;
    p.setBrush(c);
    p.setPen(Qt::NoPen);

    // 펌프 몸체
    p.drawRoundedRect(ox, oy, 16, 20, 2, 2);

    // 노즐 파이프 (오른쪽)
    QPen pen(c, 2.5f, Qt::SolidLine, Qt::RoundCap);
    p.setPen(pen);
    p.drawLine(ox + 16, oy + 4, ox + 24, oy + 4);
    p.drawLine(ox + 24, oy + 4, ox + 24, oy + 12);
    p.drawLine(ox + 22, oy + 12, ox + 26, oy + 12);

    // 연료 심볼 (몸체 안)
    p.setPen(QPen(QColor(0x08, 0x08, 0x12), 2));
    p.drawLine(ox + 4, oy + 10, ox + 12, oy + 10);
    p.drawLine(ox + 8, oy + 6,  ox + 8,  oy + 14);
}

void IndicatorWidget::drawBattery(QPainter &p, const QColor &c, int iw, int ih) {
    const int bw = 32, bh = 16;
    const int bx = (iw - bw) / 2;
    const int by = (ih - bh) / 2;

    // 배터리 몸체
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(c, 2));
    p.drawRoundedRect(bx, by, bw - 4, bh, 2, 2);

    // 양극 단자
    p.drawRect(bx + bw - 4, by + 4, 4, bh - 8);

    // 내부 +/- 표시
    p.setPen(QPen(c, 1.5f));
    int mx = bx + (bw - 4) / 2;
    int my = by + bh / 2;
    p.drawLine(mx - 8, my, mx - 4, my);        // -
    p.drawLine(mx + 4, my, mx + 8, my);        // + 수평
    p.drawLine(mx + 6, my - 2, mx + 6, my + 2); // + 수직
}
