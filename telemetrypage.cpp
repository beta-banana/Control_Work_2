#include "telemetrypage.h"
#include "telemetrychart.h"
#include <QDoubleSpinBox>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QSaveFile>
#include <QSpinBox>
#include <QStyle>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>
#include <cmath>

namespace {
QLabel* label(const QString& text, const char* name, bool wrap = false)
{
    auto* result = new QLabel(text);
    result->setObjectName(name);
    result->setTextFormat(Qt::PlainText);
    result->setWordWrap(wrap);
    return result;
}
QPushButton* button(const QString& title, const char* role)
{
    auto* result = new QPushButton(title);
    result->setProperty("role", role);
    result->setCursor(Qt::PointingHandCursor);
    return result;
}
QFrame* make_metric(const QString& title, QLabel*& value, const char* accent)
{
    auto* frame = new QFrame;
    frame->setObjectName("metric");
    auto* layout = new QVBoxLayout(frame);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(2);
    layout->addWidget(label(title, "instruction"));
    value = label({}, "metricValue");
    value->setProperty("accent", accent);
    layout->addWidget(value);
    return frame;
}
QString number(double value, int digits)
{
    return QLocale(QLocale::Russian).toString(value, 'f', digits);
}
void wait_for_events(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}
}

TelemetryPage::TelemetryPage(QWidget* parent) : QWidget(parent)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(12);
    auto* metrics = new QHBoxLayout;
    metrics->setSpacing(12);
    metrics->addWidget(make_metric("Текущая высота", altitude_, "violet"), 1);
    metrics->addWidget(make_metric("Ошибка по высоте", error_, "mint"), 1);
    metrics->addWidget(make_metric("Управляющий сигнал", command_, "pink"), 1);
    root->addLayout(metrics);

    auto* body = new QHBoxLayout;
    body->setSpacing(12);
    auto* charts = new QVBoxLayout;
    charts->setSpacing(12);
    height_chart_ = new TelemetryChart(model_, TelemetryChart::Altitude);
    control_chart_ = new TelemetryChart(model_, TelemetryChart::Control);
    charts->addWidget(height_chart_, 3);
    charts->addWidget(control_chart_, 2);
    body->addLayout(charts, 1);

    auto* controls = new QFrame;
    controls->setObjectName("control");
    controls->setFixedWidth(232);
    auto* panel = new QVBoxLayout(controls);
    panel->setContentsMargins(15, 16, 15, 16);
    panel->setSpacing(9);
    panel->addWidget(label("Параметры прогона", "sectionTitle"));
    panel->addWidget(label("Старт с 150 м · шаг 0,1 с", "instruction", true));
    panel->addSpacing(5);
    auto* target_label = label("Целевая высота", "instruction");
    panel->addWidget(target_label);
    target_ = new QSpinBox;
    target_->setRange(50, 350);
    target_->setSuffix(" м");
    target_->setValue(200);
    target_label->setBuddy(target_);
    target_->setAccessibleName("Целевая высота");
    panel->addWidget(target_);
    auto* gain_label = label("Коэффициент регулятора", "instruction");
    panel->addWidget(gain_label);
    gain_ = new QDoubleSpinBox;
    gain_->setLocale(QLocale(QLocale::Russian));
    gain_->setRange(0.10, 1.50);
    gain_->setSingleStep(0.05);
    gain_->setDecimals(2);
    gain_->setValue(0.5);
    gain_label->setBuddy(gain_);
    gain_->setAccessibleName("Коэффициент регулятора");
    panel->addWidget(gain_);
    panel->addSpacing(4);
    start_ = button("Начать прогон", "primary");
    reset_ = button("Сбросить", "secondary");
    export_ = button("Экспорт CSV", "secondary");
    panel->addWidget(start_);
    panel->addWidget(reset_);
    panel->addWidget(export_);
    panel->addStretch();
    progress_ = label({}, "instruction", true);
    panel->addWidget(progress_);
    status_ = label({}, "outcome", true);
    panel->addWidget(status_);
    body->addWidget(controls);
    root->addLayout(body, 1);

    timer_ = new QTimer(this);
    timer_->setInterval(100);
    connect(timer_, &QTimer::timeout, this, &TelemetryPage::advance);
    connect(start_, &QPushButton::clicked, this, &TelemetryPage::toggle_run);
    connect(reset_, &QPushButton::clicked, this, &TelemetryPage::reset_run);
    connect(export_, &QPushButton::clicked, this, &TelemetryPage::export_csv);
    connect(target_, &QSpinBox::valueChanged, this, &TelemetryPage::reset_run);
    connect(gain_, &QDoubleSpinBox::valueChanged, this, &TelemetryPage::reset_run);
    reset_run();
}

void TelemetryPage::toggle_run()
{
    if (timer_->isActive()) {
        timer_->stop();
    } else {
        if (started_ && model_.finished()) reset_run();
        started_ = true;
        if (!model_.finished()) timer_->start();
    }
    refresh();
}

void TelemetryPage::reset_run()
{
    timer_->stop();
    started_ = false;
    model_.reset(target_->value(), gain_->value());
    refresh();
}

void TelemetryPage::advance()
{
    model_.step();
    if (model_.finished()) timer_->stop();
    refresh();
}

void TelemetryPage::refresh()
{
    const auto& sample = model_.samples().back();
    altitude_->setText(number(sample.altitude, 1) + " м");
    error_->setText(number(sample.target - sample.altitude, 1) + " м");
    command_->setText(number(sample.control, 2));
    progress_->setText("Время " + number(sample.time, 1) + " с\nЗаписей: " + QString::number(model_.samples().size()));
    QString caption;
    QString tone = "neutral";
    if (!started_) {
        caption = "Готов к запуску";
        start_->setText("Начать прогон");
    } else if (model_.finished()) {
        caption = model_.reached_target() ? "Цель достигнута" : "Лимит 20 с достигнут";
        tone = model_.reached_target() ? "ok" : "error";
        start_->setText("Новый прогон");
    } else if (timer_->isActive()) {
        caption = "Симуляция идёт";
        tone = "ok";
        start_->setText("Пауза");
    } else {
        caption = "На паузе";
        start_->setText("Продолжить");
    }
    status_->setText(caption);
    if (status_->property("tone") != tone) {
        status_->setProperty("tone", tone);
        status_->style()->unpolish(status_);
        status_->style()->polish(status_);
    }
    target_->setEnabled(!started_ || model_.finished());
    gain_->setEnabled(!started_ || model_.finished());
    export_->setEnabled(model_.samples().size() > 1);
    height_chart_->update();
    control_chart_->update();
}

bool TelemetryPage::save_csv(const QString& path) const
{
    QSaveFile file(path);
    const auto bytes = model_.csv();
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
}

void TelemetryPage::export_csv()
{
    auto path = QFileDialog::getSaveFileName(this, "Сохранить телеметрию", "telemetry.csv", "CSV (*.csv)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".csv", Qt::CaseInsensitive)) path += ".csv";
    const bool saved = save_csv(path);
    QToolTip::showText(export_->mapToGlobal(QPoint(0, export_->height())),
                       saved ? "Телеметрия сохранена" : "Не удалось записать CSV", export_);
}

bool TelemetryPage::verify_interactions()
{
    target_->setValue(100);
    gain_->setValue(0.75);
    start_->click();
    wait_for_events(350);
    start_->click();
    const auto count = model_.samples().size();
    if (timer_->isActive() || count < 2 || model_.samples().back().altitude >= 150
        || !export_->isEnabled()) return false;
    wait_for_events(140);
    if (model_.samples().size() != count) return false;
    start_->click();
    if (!timer_->isActive()) return false;
    QTemporaryDir directory;
    const auto path = directory.filePath("telemetry.csv");
    if (!directory.isValid() || !save_csv(path)) return false;
    QFile csv(path);
    if (!csv.open(QIODevice::ReadOnly) || csv.readAll() != model_.csv()) return false;
    reset_->click();
    if (timer_->isActive() || model_.samples().size() != 1 || export_->isEnabled()) return false;

    target_->setValue(200);
    gain_->setValue(0.5);
    start_->click();
    while (!model_.finished()) advance();
    return !timer_->isActive() && status_->text() == "Цель достигнута"
           && std::abs(model_.samples().back().altitude - 200) <= 0.5;
}
