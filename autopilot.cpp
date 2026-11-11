#include "autopilot.h"
#include "uav.h"
#include "sensor.h"
#include "navigation.h"
#include "controller.h"
#include <QDebug>
#include <cmath>

Autopilot::Autopilot(UAV* vehicle, const std::vector<Sensor*>& sensors,
                     NavigationSystem* navigation, FlightController* controller)
    : vehicle_(vehicle), sensors_(sensors), navigation_(navigation), controller_(controller) {}

void Autopilot::abort_cycle(const QString& reason)
{
    result_.error = reason;
    qDebug().noquote() << "Autopilot cycle aborted";
    qDebug().noquote() << "Reason:" << reason;
}

void Autopilot::run_cycle()
{
    result_ = {};
    qDebug() << "Autopilot cycle started";
    if (!vehicle_) return abort_cycle("Error: vehicle is not set");
    qDebug().noquote() << "Vehicle:" << vehicle_->type();
    if (sensors_.empty()) return abort_cycle("sensor failure");
    // Для этой учебной конфигурации все переданные сенсоры критичны.
    for (auto* sensor : sensors_) {
        if (!sensor) {
            qDebug() << "Error: sensor pointer is null";
            return abort_cycle("sensor failure");
        }
        if (!sensor->is_ready()) {
            qDebug().noquote() << sensor->name() + ": not ready";
            return abort_cycle("sensor failure");
        }
        const double value = sensor->read();
        if (!std::isfinite(value)) return abort_cycle("sensor failure");
        qDebug().noquote() << sensor->name() + ": Ready, value=" + QString::number(value);
    }
    qDebug() << "All sensors are ready";
    if (!navigation_) return abort_cycle("Error: navigation is not set");
    if (!navigation_->is_valid()) return abort_cycle("Navigation data invalid");
    result_.navigation = navigation_->system_name();
    qDebug().noquote() << "Navigation:" << result_.navigation;
    const Position p = navigation_->get_position();
    if (!std::isfinite(p.latitude) || !std::isfinite(p.longitude) || !std::isfinite(p.altitude)
        || std::abs(p.latitude) > 90 || std::abs(p.longitude) > 180 || p.altitude < 0) {
        return abort_cycle("Error: invalid navigation position");
    }
    result_.current_altitude = p.altitude;
    result_.altitude_error = result_.target_altitude - p.altitude;
    qDebug().noquote() << "Current altitude:" << p.altitude << "m";
    qDebug().noquote() << "Target altitude:" << result_.target_altitude << "m";
    qDebug().noquote() << "Altitude error:" << *result_.altitude_error << "m";
    if (!controller_) return abort_cycle("Error: flight controller is not set");
    const double output = controller_->calculate_control(*result_.altitude_error, 0.1);
    if (!std::isfinite(output)) return abort_cycle("Error: invalid controller output");
    result_.output = output;
    result_.completed = true;
    qDebug().noquote() << "Controller output:" << output;
    qDebug() << "Autopilot cycle completed";
}

const CycleResult& Autopilot::last_result() const { return result_; }
