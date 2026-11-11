#pragma once
#include <QMainWindow>
#include <QStringList>
#include <array>
#include "uav.h"
#include "sensor.h"
#include "navigation.h"
#include "autopilot.h"

class QLabel;
class QFrame;
class QComboBox;
class QCheckBox;
class QPushButton;
class QPlainTextEdit;
class QStackedWidget;
class QButtonGroup;
class Instruments;
class TelemetryPage;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    bool verify_and_capture(const QString& directory);
private:
    enum Page { Airframe, Sensors, Navigation, Cycle, Telemetry };
    void switch_page(int page);
    void refresh_airframe(bool test = false);
    void refresh_sensors();
    void refresh_navigation();
    void prepare_cycle();
    void run_cycle();
    void update_journal(const QString& text);
    void show_outcome(const QString& message, const char* tone = "ok");

    Quadcopter quad_;
    Hexacopter hexa_;
    Octocopter octo_;
    const std::array<UAV*, 3> fleet_{{&quad_, &hexa_, &octo_}};
    GPS gps_;
    GLONASS glonass_;
    VisualOdometry visual_;
    const std::vector<Sensor*> sensors_{&gps_, &glonass_, &visual_};
    GPSNavigation gps_nav_;
    InertialNavigation ins_;
    VisualNavigation visual_nav_;
    const std::vector<NavigationSystem*> nav_sources_{&gps_nav_, &ins_, &visual_nav_};
    NavigationManager manager_{nav_sources_};

    QButtonGroup* routes_;
    QLabel* title_;
    QLabel* description_;
    QLabel* outcome_;
    Instruments* instruments_;
    QPlainTextEdit* journal_;
    QStackedWidget* controls_;
    QWidget* exercise_;
    QFrame* journal_panel_;
    TelemetryPage* telemetry_;
    QComboBox* aircraft_;
    QComboBox* scenario_;
    QPushButton* flight_button_;
    QPushButton* cycle_button_;
    std::array<QCheckBox*, 3> sensor_ready_{};
    std::array<QCheckBox*, 3> nav_valid_{};
    CycleResult cycle_result_;
    QString cycle_log_;
    bool cycle_has_run_ = false;
};
