#include "selftest.h"
#include "uav.h"
#include "sensor.h"
#include "navigation.h"
#include "controller.h"
#include "autopilot.h"
#include "debuglog.h"
#include "telemetrymodel.h"

#include <QDebug>
#include <cmath>
#include <limits>
#include <type_traits>

int run_self_tests()
{
    static_assert(std::is_abstract_v<UAV> && std::is_abstract_v<Sensor>
                  && std::is_abstract_v<NavigationSystem> && std::is_abstract_v<FlightController>);
    static_assert(std::has_virtual_destructor_v<UAV> && std::has_virtual_destructor_v<Sensor>
                  && std::has_virtual_destructor_v<NavigationSystem>
                  && std::has_virtual_destructor_v<FlightController>);
    DebugLog log;
    int failed = 0;
    int total = 0;
    const auto expect = [&](bool ok, const char* title) {
        ++total;
        if (!ok) { ++failed; qWarning().noquote() << "FAIL:" << title; }
    };
    Quadcopter quad;
    Hexacopter hexa;
    Octocopter octo;
    const std::vector<const UAV*> fleet{&quad, &hexa, &octo};
    for (auto* v : fleet) print_vehicle_info(*v);
    expect(fleet[0]->motor_count() == 4 && fleet[1]->motor_count() == 6
           && fleet[2]->motor_count() == 8, "motor count through base pointers");
    expect(log.text().contains("Quadcopter: 4 motors") && log.text().contains("Landing: vertical landing"),
           "flight profile output");

    GPS gps;
    GLONASS glonass;
    VisualOdometry visual;
    std::vector<Sensor*> sensors{nullptr, &visual, &glonass, &gps};
    expect(choose_best_sensor(sensors) == &gps, "minimum error across ready sensors");
    gps.set_ready(false);
    expect(choose_best_sensor(sensors) == &glonass, "GPS out: GLONASS selected");
    glonass.set_ready(false);
    expect(choose_best_sensor(sensors) == &visual, "GLONASS out: visual selected");
    visual.set_ready(false);
    expect(choose_best_sensor(sensors) == nullptr, "no ready sensors");
    expect(log.text().contains("Error: no navigation sensor is ready"), "no sensor error logged");
    for (Sensor* s : std::vector<Sensor*>{&gps, &glonass, &visual}) s->set_ready(true);

    GPSNavigation gpsNav;
    InertialNavigation ins;
    VisualNavigation visualNav;
    NavigationManager manager({&gpsNav, nullptr, &ins, &visualNav});
    expect(manager.system_name() == "GPS", "GPS priority");
    gpsNav.set_valid(false);
    expect(manager.is_available() && manager.system_name() == "INS", "automatic GPS to INS");
    ins.set_valid(false);
    expect(manager.system_name() == "Visual", "automatic INS to Visual");
    visualNav.set_valid(false);
    expect(!manager.is_available() && manager.active_system() == nullptr, "all navigation failed");
    expect(std::isnan(manager.get_position().altitude), "no fictitious altitude on outage");
    gpsNav.set_valid(true);
    gpsNav.set_satellites(3);
    expect(!manager.is_available(), "insufficient satellites");
    gpsNav.set_satellites(10);
    expect(manager.system_name() == "GPS", "automatic return to GPS");

    NavigationManager manual;
    manual.set_system(&gpsNav);
    expect(manual.active_system() == &gpsNav, "single system via set_system");
    manual.set_system(nullptr);
    expect(!manual.is_available(), "null system via set_system");

    SimpleController controller;
    const std::vector<Sensor*> critical{&gps, &glonass, &visual};
    Autopilot autopilot(&quad, critical, &manager, &controller);
    const auto run = [&](Autopilot& ap) { ap.run_cycle(); return ap.last_result(); };
    auto result = run(autopilot);
    expect(result.completed && result.current_altitude == 150 && result.altitude_error == 50
           && result.output == 25, "normal cycle: 150 -> 200, error 50, output 25");
    gps.set_ready(false);
    expect(run(autopilot).error == "sensor failure", "not ready sensor aborts");
    gps.set_ready(true);
    gpsNav.set_valid(false);
    expect(run(autopilot).error == "Navigation data invalid", "invalid navigation aborts");
    gpsNav.set_valid(true);
    Autopilot missingController(&quad, critical, &manager, nullptr);
    expect(run(missingController).error == "Error: flight controller is not set", "null controller");
    gpsNav.set_position({55.75, 37.61, -10});
    expect(run(autopilot).error == "Error: invalid navigation position", "negative altitude");
    gpsNav.set_position({95, 37.61, 150});
    expect(run(autopilot).error == "Error: invalid navigation position", "out of range latitude");
    gpsNav.set_position({55.75, 37.61, 150});
    Autopilot missingVehicle(nullptr, critical, &manager, &controller);
    expect(run(missingVehicle).error == "Error: vehicle is not set", "null vehicle");
    Autopilot missingSensor(&quad, {nullptr}, &manager, &controller);
    expect(run(missingSensor).error == "sensor failure", "null sensor");
    Autopilot missingNavigation(&quad, critical, nullptr, &controller);
    expect(run(missingNavigation).error == "Error: navigation is not set", "null navigation");
    expect(log.text().contains("Autopilot cycle completed")
           && log.text().contains("Reason: sensor failure"), "normal and aborted cycle messages");
    TelemetryModel telemetry;
    expect(telemetry.samples().size() == 1 && telemetry.samples().back().altitude == 150
           && telemetry.samples().back().control == 25, "telemetry starts at 150 m, command 25");
    telemetry.step();
    expect(telemetry.samples().size() == 2 && telemetry.samples().back().time == 0.1
           && telemetry.samples().back().altitude == 152.5, "one 0.1 second step");
    while (!telemetry.finished()) telemetry.step();
    expect(telemetry.reached_target() && std::abs(telemetry.samples().back().altitude - 200) <= 0.5,
           "climb reaches the target tolerance");
    const auto count = telemetry.samples().size();
    telemetry.step();
    expect(telemetry.samples().size() == count && telemetry.csv().count('\n') == count + 1,
           "finished run freezes, CSV exports every sample");
    telemetry.reset(100, 0.5);
    telemetry.step();
    expect(telemetry.samples().back().altitude == 147.5
           && telemetry.samples().back().control < 0, "reset and descent");
    telemetry.reset(350, 0.1);
    while (!telemetry.finished()) telemetry.step();
    expect(!telemetry.reached_target() && telemetry.samples().size() == 201
           && telemetry.samples().back().time == 20.0, "20 second limit for slow regulator");
    qInfo().noquote() << QString("Self-test: %1/%2 passed").arg(total-failed).arg(total);
    return failed != 0;
}
