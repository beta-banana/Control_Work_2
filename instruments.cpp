#include "instruments.h"
#include <QPainter>
#include <QFontDatabase>
#include <QLocale>
#include <cmath>
#include <algorithm>
#include <vector>

namespace {
const QColor ink(32, 28, 48), rule(61, 53, 82), mint(179, 153, 255);
const QColor white(247, 242, 255), muted(182, 174, 202), amber(250, 182, 136);
const QColor slate(105, 89, 143), cell(46, 38, 66);

void text(QPainter& p, QRectF r, const QString& value, int size, const QColor& color,
          int weight = QFont::Normal, Qt::Alignment align = Qt::AlignLeft | Qt::AlignVCenter)
{
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPixelSize(size);
    font.setWeight(static_cast<QFont::Weight>(weight));
    p.setFont(font);
    p.setPen(color);
    p.drawText(r, align, value);
}
void line(QPainter& p, QPointF a, QPointF b, const QColor& color, double width = 1)
{
    p.setPen(QPen(color, width, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(a, b);
}
void box(QPainter& p, QRectF r, const QColor& color, qreal radius = 8)
{
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawRoundedRect(r, radius, radius);
}
void dot(QPainter& p, QPointF at, qreal radius, const QColor& color)
{
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    p.drawEllipse(at, radius, radius);
}
QString decimal(double value, int precision)
{
    return QLocale(QLocale::Russian).toString(value, 'f', precision);
}
void header(QPainter& p, const QString& title, const QString& detail)
{
    text(p, {28, 18, 350, 24}, title, 15, white, QFont::DemiBold);
    text(p, {355, 18, 236, 24}, detail, 12, muted, QFont::Normal, Qt::AlignRight | Qt::AlignVCenter);
    line(p, {28, 49}, {592, 49}, rule);
}
}

Instruments::Instruments(QWidget* parent) : QWidget(parent)
{
    setMinimumSize(460, 305);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName("Схема текущего испытания");
}
void Instruments::show_airframe(int motors, const QString& name)
{
    mode_ = Mode::Airframe; motors_ = motors; aircraft_ = name; update();
}
void Instruments::show_sensors(const std::array<bool, 3>& ready, const std::array<double, 3>& accuracy, int best)
{
    mode_ = Mode::Sensors; available_ = ready; errors_ = accuracy; active_ = best; update();
}
void Instruments::show_navigation(const std::array<bool, 3>& valid, int active, const QString& coordinates)
{
    mode_ = Mode::Navigation; available_ = valid; active_ = active; coordinates_ = coordinates; update();
}
void Instruments::show_cycle(const CycleResult& result, bool hasRun)
{
    mode_ = Mode::Cycle; cycle_ = result; has_run_ = hasRun; update();
}
void Instruments::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    box(p, rect(), ink, 14);
    constexpr double designW = 620, designH = 350;
    const double scale = std::min(width() / designW, height() / designH);
    p.translate((width() - designW * scale) / 2, (height() - designH * scale) / 2);
    p.scale(scale, scale);
    switch (mode_) {
        case Mode::Airframe: draw_airframe(p); break;
        case Mode::Sensors: draw_sensors(p); break;
        case Mode::Navigation: draw_navigation(p); break;
        case Mode::Cycle: draw_cycle(p); break;
    }
}

void Instruments::draw_airframe(QPainter& p)
{
    header(p, "Схема аппарата", "Вид сверху");
    const QPointF center(220, 193);
    p.setPen(QPen(rule, 1, Qt::DotLine));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(center, 118, 118);
    line(p, {79, 193}, {361, 193}, rule);
    line(p, {220, 58}, {220, 328}, rule);
    std::vector<QPointF> positions;
    constexpr double pi = 3.141592653589793;
    for (int i = 0; i < motors_; ++i) {
        const double angle = 2 * pi * i / motors_ - pi / 2 + pi / motors_;
        const QPointF point = center + QPointF(102 * std::cos(angle), 102 * std::sin(angle));
        positions.push_back(point);
        line(p, center, point, slate, 5);
    }
    box(p, {193, 166, 54, 54}, cell, 11);
    dot(p, center, 5, mint);
    for (size_t i = 0; i < positions.size(); ++i) {
        const auto at = positions[i];
        dot(p, at, 20, ink);
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(mint, 1.6));
        p.drawEllipse(at, 19, 19);
        text(p, {at.x()-17, at.y()-13, 34, 26}, QString::number(i+1), 13, white,
             QFont::DemiBold, Qt::AlignCenter);
    }
    line(p, {386, 88}, {386, 296}, rule);
    text(p, {412, 99, 178, 24}, aircraft_, 17, white, QFont::DemiBold);
    text(p, {409, 135, 171, 72}, QString::number(motors_), 65, mint, QFont::DemiBold);
    text(p, {413, 209, 167, 22}, motors_ == 4 ? "мотора" : "моторов", 15, muted);
    text(p, {413, 252, 177, 20}, "Вертикальный взлёт", 13, white);
    text(p, {413, 274, 177, 20}, "Вертикальная посадка", 13, white);
}

void Instruments::draw_sensors(QPainter& p)
{
    header(p, "Сравнение источников", "Меньше ошибка — точнее");
    const std::array<QString, 3> names{"GPS", "ГЛОНАСС", "Визуальная одометрия"};
    for (int i = 0; i < 3; ++i) {
        const double y = 72 + i * 72;
        text(p, {28, y, 240, 23}, names[i], 15, white, QFont::DemiBold);
        const auto value = available_[i] ? decimal(errors_[i], 1) + " м" : QString("Нет данных");
        text(p, {492, y, 100, 23}, value, 14, available_[i] ? white : muted,
             QFont::Normal, Qt::AlignRight | Qt::AlignVCenter);
        box(p, {28, y+29, 564, 10}, rule, 5);
        if (available_[i]) box(p, {28, y+29, 564*errors_[i]/4.0, 10}, i == active_ ? mint : slate, 5);
    }
    const QString summary = active_ < 0 ? "Ни один сенсор не готов" : "Выбран источник: " + names[active_];
    dot(p, {35, 319}, 4, active_ < 0 ? amber : mint);
    text(p, {49, 305, 543, 28}, summary, 15, active_ < 0 ? amber : mint, QFont::DemiBold);
}

void Instruments::draw_navigation(QPainter& p)
{
    header(p, "Цепочка резервирования", "Приоритет слева направо");
    const std::array<QString, 3> names{"GPS", "INS", "Камера"};
    const std::array<QString, 3> labels{"Спутники", "Инерциальная", "Визуальная"};
    for (int i = 0; i < 3; ++i) {
        const double x = 28 + i * 196;
        const bool active = i == active_;
        const QColor color = active ? mint : available_[i] ? white : amber;
        box(p, {x, 98, 172, 128}, cell, 9);
        box(p, {x+14, 113, 22, 20}, active ? mint : rule, 4);
        text(p, {x+14, 113, 22, 20}, QString::number(i+1), 11, active ? ink : white,
             QFont::Bold, Qt::AlignCenter);
        text(p, {x+14, 143, 146, 28}, names[i], 23, color, QFont::DemiBold);
        text(p, {x+14, 174, 146, 21}, labels[i], 12, muted);
        text(p, {x+14, 199, 146, 21}, active ? "Активна" : available_[i] ? "В резерве" : "Недоступна",
             13, color);
        if (i < 2) {
            line(p, {x+178, 163}, {x+189, 163}, slate, 1.5);
            line(p, {x+185, 159}, {x+189, 163}, slate, 1.5);
            line(p, {x+185, 167}, {x+189, 163}, slate, 1.5);
        }
    }
    text(p, {28, 250, 564, 26}, active_ < 0 ? "Критическая ошибка: навигация недоступна"
                : "Активная система: " + names[active_], 17, active_ < 0 ? amber : mint, QFont::DemiBold);
    text(p, {28, 283, 564, 26}, coordinates_, 13, muted);
}

void Instruments::draw_cycle(QPainter& p)
{
    header(p, "Контур высоты", "Цель 200 м   /   Шаг 0,1 с");
    const double top = 79, bottom = 302, x = 160;
    const auto y_for = [&](double meters) { return bottom - (bottom-top)*meters/250.; };
    for (int h = 0; h <= 250; h += 50) {
        const double y = y_for(h);
        line(p, {28, y}, {356, y}, rule);
        text(p, {29, y-15, 60, 14}, QString::number(h) + " м", 11, muted);
    }
    line(p, {x, top}, {x, bottom}, slate, 4);
    const double goal = y_for(cycle_.target_altitude);
    dot(p, {x, goal}, 6, mint);
    text(p, {179, goal-22, 174, 20}, "Цель · 200 м", 14, mint, QFont::DemiBold);
    if (cycle_.current_altitude) {
        const double current = y_for(std::clamp(*cycle_.current_altitude, 0.0, 250.0));
        dot(p, {x, current}, 6, white);
        text(p, {179, current+4, 174, 20},
             "Текущая · " + decimal(*cycle_.current_altitude, 0) + " м", 13, white);
    }
    line(p, {385, 71}, {385, 309}, rule);
    const QColor color = !has_run_ ? muted : cycle_.completed ? mint : amber;
    text(p, {410, 86, 190, 26}, !has_run_ ? "Ожидает запуска" : cycle_.completed ? "Цикл завершён" : "Цикл прерван",
         17, color, QFont::DemiBold);
    text(p, {410, 134, 192, 21}, "Ошибка по высоте", 13, muted);
    text(p, {410, 156, 192, 37}, cycle_.altitude_error ? decimal(*cycle_.altitude_error, 0) + " м" : "—",
         28, white, QFont::DemiBold);
    text(p, {410, 212, 192, 21}, "Сигнал управления", 13, muted);
    text(p, {410, 234, 192, 42}, cycle_.output ? decimal(*cycle_.output, 0) : "—",
         32, color, QFont::DemiBold);
}
