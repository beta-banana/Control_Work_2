#include "telemetrymodel.h"
#include <cmath>

TelemetryModel::TelemetryModel() { reset(200.0, 0.5); }

void TelemetryModel::reset(double target, double gain)
{
    controller_ = SimpleController(gain);
    samples_.clear();
    constexpr double initial = 150.0;
    samples_.append({0, initial, target, controller_.calculate_control(target - initial, dt_)});
}

void TelemetryModel::step()
{
    if (finished()) return;
    const auto previous = samples_.back();
    const double altitude = previous.altitude + previous.control * dt_;
    const double time = samples_.size() * dt_;
    samples_.append({time, altitude, previous.target,
                     controller_.calculate_control(previous.target - altitude, dt_)});
}

bool TelemetryModel::reached_target() const
{
    const auto& sample = samples_.back();
    return std::abs(sample.target - sample.altitude) <= tolerance_;
}

bool TelemetryModel::finished() const { return reached_target() || samples_.size() > max_steps_; }
const QVector<TelemetrySample>& TelemetryModel::samples() const { return samples_; }

QByteArray TelemetryModel::csv() const
{
    QByteArray output("time_s,altitude_m,target_m,control\n");
    for (const auto& sample : samples_) {
        output += QByteArray::number(sample.time, 'f', 1) + ','
                + QByteArray::number(sample.altitude, 'f', 3) + ','
                + QByteArray::number(sample.target, 'f', 1) + ','
                + QByteArray::number(sample.control, 'f', 3) + '\n';
    }
    return output;
}
