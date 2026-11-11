#include "sensor.h"
#include <QDebug>
#include <cmath>

bool NavigationSensor::is_ready() const { return ready_; }
void NavigationSensor::set_ready(bool ready) { ready_ = ready; }

double GPS::read() { return 10; }
QString GPS::name() const { return "GPS"; }
double GPS::accuracy() const { return 1.5; }
QString GPS::reading_text() { return QString("satellites=%1").arg(read()); }

double GLONASS::read() { return 8; }
QString GLONASS::name() const { return "GLONASS"; }
double GLONASS::accuracy() const { return 2.0; }
QString GLONASS::reading_text() { return QString("satellites=%1").arg(read()); }

double VisualOdometry::read() { return 3.2; }
QString VisualOdometry::name() const { return "Visual odometry"; }
double VisualOdometry::accuracy() const { return 3.2; }
QString VisualOdometry::reading_text() { return {}; }

Sensor* choose_best_sensor(const std::vector<Sensor*>& sensors)
{
    Sensor* best = nullptr;
    for (auto* sensor : sensors) {
        if (!sensor) {
            qDebug() << "Sensor pointer is null";
            continue;
        }
        if (!sensor->is_ready()) {
            qDebug().noquote() << sensor->name() + ": not ready";
            continue;
        }
        const auto error = sensor->accuracy();
        if (!std::isfinite(error) || error < 0) {
            qDebug().noquote() << sensor->name() + ": invalid accuracy";
            continue;
        }
        const auto details = sensor->reading_text();
        qDebug().noquote() << QString("%1: %2accuracy=%3 m")
                              .arg(sensor->name(), details.isEmpty() ? QString() : details + ", ")
                              .arg(error, 0, 'f', 1);
        if (!best || error < best->accuracy())
            best = sensor;
    }
    if (best) qDebug().noquote() << "Best sensor:" << best->name();
    else qDebug() << "Error: no navigation sensor is ready";
    return best;
}
