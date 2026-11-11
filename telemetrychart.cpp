#include "telemetrychart.h"
#include <QFontDatabase>
#include <QLinearGradient>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

namespace {
const QColor surface(32, 28, 48), border(61, 53, 83), grid(61, 53, 82);
const QColor foreground(247, 242, 255), muted(182, 174, 202);
const QColor violet(179, 153, 255), pink(246, 153, 197), mint(117, 224, 197);
QString number(double v, int digits) { return QLocale(QLocale::Russian).toString(v, 'f', digits); }

void draw_text(QPainter& p, QRectF rect, const QString& text, int px, const QColor& color,
               Qt::Alignment alignment = Qt::AlignVCenter | Qt::AlignLeft, bool bold = false)
{
    auto font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPixelSize(px);
    font.setWeight(bold ? QFont::DemiBold : QFont::Normal);
    p.setFont(font);
    p.setPen(color);
    p.drawText(rect, alignment, text);
}

double nice_step(double span)
{
    const double raw = span / 4;
    const double power = std::pow(10.0, std::floor(std::log10(raw)));
    const double normalized = raw / power;
    return (normalized <= 1 ? 1 : normalized <= 2 ? 2 : normalized <= 5 ? 5 : 10) * power;
}
}

TelemetryChart::TelemetryChart(const TelemetryModel& model, Series series, QWidget* parent)
    : QWidget(parent), model_(model), series_(series)
{
    setMinimumSize(365, series == Altitude ? 204 : 174);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    setAccessibleName(series == Altitude ? "График высоты" : "График управления");
}

void TelemetryChart::mouseMoveEvent(QMouseEvent* event)
{
    pointer_ = event->position();
    update();
}

void TelemetryChart::leaveEvent(QEvent*)
{
    pointer_ = {-1, -1};
    update();
}

void TelemetryChart::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(border);
    p.setBrush(surface);
    p.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 14, 14);

    const bool altitude = series_ == Altitude;
    const QColor accent = altitude ? violet : pink;
    const auto& samples = model_.samples();
    const auto value = [altitude](const TelemetrySample& s) { return altitude ? s.altitude : s.control; };
    const auto& first = samples.front();
    double low = altitude ? std::min(first.altitude, first.target) : std::min(0.0, first.control);
    double high = altitude ? std::max(first.altitude, first.target) : std::max(0.0, first.control);
    for (const auto& sample : samples) {
        low = std::min(low, value(sample));
        high = std::max(high, value(sample));
    }
    const double padding = std::max(altitude ? 6.0 : 2.0, (high - low) * 0.12);
    const double step = nice_step(high - low + 2 * padding);
    const double bottom = std::floor((low - padding) / step) * step;
    const double top = std::ceil((high + padding) / step) * step;
    const double duration = std::max(10.0, std::ceil(samples.back().time / 5.0) * 5);
    const QRectF plot(56, 62, width() - 80.0, height() - 94.0);
    const auto x = [&](double t) { return plot.left() + plot.width() * t / duration; };
    const auto y = [&](double v) { return plot.bottom() - plot.height() * (v - bottom) / (top - bottom); };

    draw_text(p, {18, 12, width() - 36.0, 25}, altitude ? "Высота, м" : "Управляющий сигнал, усл. ед.",
              16, foreground, Qt::AlignVCenter | Qt::AlignLeft, true);
    const auto legend = [&](double left, const QString& title, const QColor& color, Qt::PenStyle style) {
        p.setPen(QPen(color, 2, style, Qt::RoundCap));
        p.drawLine(QPointF(left, 49), QPointF(left + 13, 49));
        draw_text(p, {left + 20, 40, 112, 18}, title, 11, muted);
    };
    legend(19, altitude ? "Текущая" : "Команда", accent, Qt::SolidLine);
    if (altitude) legend(125, "Целевая", mint, Qt::DashLine);

    for (double tick = bottom; tick <= top + 1e-8; tick += step) {
        p.setPen(QPen(grid, 1));
        p.drawLine(QPointF(plot.left(), y(tick)), QPointF(plot.right(), y(tick)));
        draw_text(p, {10, y(tick) - 8, 38, 16}, number(std::abs(tick) < 1e-8 ? 0 : tick, step < 1 ? 1 : 0),
                  11, muted, Qt::AlignRight | Qt::AlignVCenter);
    }
    for (int i = 0; i <= 5; ++i) {
        const double t = duration * i / 5;
        draw_text(p, {x(t) - 21, plot.bottom() + 7, 42, 18}, number(t, 0) + " с", 11, muted, Qt::AlignCenter);
    }

    p.save();
    p.setClipRect(plot.adjusted(-4, -4, 4, 4));
    if (altitude) {
        p.setPen(QPen(mint, 1.4, Qt::DashLine));
        p.drawLine(QPointF(plot.left(), y(first.target)), QPointF(plot.right(), y(first.target)));
    } else {
        p.setPen(QPen(border.lighter(145), 1));
        p.drawLine(QPointF(plot.left(), y(0)), QPointF(plot.right(), y(0)));
    }
    QPainterPath trace;
    trace.moveTo(x(first.time), y(value(first)));
    for (qsizetype i = 1; i < samples.size(); ++i)
        trace.lineTo(x(samples[i].time), y(value(samples[i])));
    if (samples.size() > 1) {
        QPainterPath area = trace;
        const double baseline = altitude ? plot.bottom() : y(0);
        area.lineTo(x(samples.back().time), baseline);
        area.lineTo(x(first.time), baseline);
        area.closeSubpath();
        QColor transparent = accent;
        transparent.setAlpha(8);
        QColor tint = accent;
        tint.setAlpha(84);
        QLinearGradient gradient(0, y(high), 0, y(low) + 1);
        gradient.setColorAt(0, tint);
        gradient.setColorAt(1, transparent);
        p.fillPath(area, gradient);
        p.setPen(QPen(accent, 2.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(trace);
    }
    p.setPen(Qt::NoPen);
    p.setBrush(accent);
    p.drawEllipse(QPointF(x(samples.back().time), y(value(samples.back()))), 3.6, 3.6);
    p.restore();

    if (plot.contains(pointer_) && pointer_.x() <= x(samples.back().time) + 4 && samples.size() > 1) {
        const auto index = std::clamp<qsizetype>(std::lround((pointer_.x()-plot.left()) * duration / plot.width() * 10),
                                                0, samples.size() - 1);
        const auto& s = samples[index];
        const double cursor = x(s.time);
        p.setPen(QPen(muted, 1, Qt::DotLine));
        p.drawLine(QPointF(cursor, plot.top()), QPointF(cursor, plot.bottom()));
        p.setPen(QPen(surface, 2));
        p.setBrush(accent);
        p.drawEllipse(QPointF(cursor, y(value(s))), 4.5, 4.5);
        const double bubbleWidth = 152;
        const double left = cursor + bubbleWidth + 12 <= plot.right() ? cursor + 10 : std::max(12.0, cursor - bubbleWidth - 10);
        const QRectF bubble(left, plot.top() + 6, bubbleWidth, 45);
        p.setPen(border.lighter(140));
        p.setBrush(QColor(47, 40, 67));
        p.drawRoundedRect(bubble, 7, 7);
        draw_text(p, {left + 10, bubble.top() + 3, 132, 17}, "Время  " + number(s.time, 1) + " с", 11, muted);
        draw_text(p, {left + 10, bubble.top() + 22, 132, 17},
                  (altitude ? "Высота  " : "Команда  ") + number(value(s), 2) + (altitude ? " м" : ""), 12, foreground);
    }
}
