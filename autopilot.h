#pragma once
#include <QString>
#include <vector>
#include <optional>
class UAV;
class Sensor;
class NavigationSystem;
class FlightController;

struct CycleResult {
    bool completed = false;
    QString error;
    QString navigation;
    std::optional<double> current_altitude;
    double target_altitude = 200;
    std::optional<double> altitude_error;
    std::optional<double> output;
};

class Autopilot {
public:
    Autopilot(UAV* vehicle, const std::vector<Sensor*>& sensors,
              NavigationSystem* navigation, FlightController* controller);
    void run_cycle();
    const CycleResult& last_result() const;
private:
    void abort_cycle(const QString& reason);
    // Модули принадлежат вызывающему коду и живут дольше этого цикла.
    UAV* vehicle_;
    std::vector<Sensor*> sensors_;
    NavigationSystem* navigation_;
    FlightController* controller_;
    CycleResult result_;
};
