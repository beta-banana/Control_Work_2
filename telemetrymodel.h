#pragma once
#include "controller.h"
#include <QByteArray>
#include <QVector>

struct TelemetrySample {
    double time;
    double altitude;
    double target;
    double control;
};

// Учебная модель: управляющий сигнал задаёт вертикальную скорость.
class TelemetryModel {
public:
    TelemetryModel();
    void reset(double target, double gain);
    void step();
    bool reached_target() const;
    bool finished() const;
    const QVector<TelemetrySample>& samples() const;
    QByteArray csv() const;
private:
    static constexpr double dt_ = 0.1;
    static constexpr int max_steps_ = 200;
    static constexpr double tolerance_ = 0.5;
    SimpleController controller_;
    QVector<TelemetrySample> samples_;
};
