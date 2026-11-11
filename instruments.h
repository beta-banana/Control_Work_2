#pragma once
#include <QWidget>
#include <array>
#include "autopilot.h"

class QPainter;
class Instruments final : public QWidget {
public:
    explicit Instruments(QWidget* parent = nullptr);
    void show_airframe(int motors, const QString& name);
    void show_sensors(const std::array<bool, 3>& ready, const std::array<double, 3>& accuracy, int best);
    void show_navigation(const std::array<bool, 3>& valid, int active, const QString& coordinates);
    void show_cycle(const CycleResult& result, bool hasRun);
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    enum class Mode { Airframe, Sensors, Navigation, Cycle };
    void draw_airframe(QPainter& p);
    void draw_sensors(QPainter& p);
    void draw_navigation(QPainter& p);
    void draw_cycle(QPainter& p);
    Mode mode_ = Mode::Airframe;
    int motors_ = 4;
    QString aircraft_;
    std::array<bool, 3> available_{};
    std::array<double, 3> errors_{};
    int active_ = -1;
    QString coordinates_;
    CycleResult cycle_;
    bool has_run_ = false;
};
