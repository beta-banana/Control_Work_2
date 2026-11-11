#pragma once
#include <QWidget>
#include "telemetrymodel.h"

class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class QLabel;
class QTimer;
class TelemetryChart;

class TelemetryPage final : public QWidget {
public:
    explicit TelemetryPage(QWidget* parent = nullptr);
    bool verify_interactions();
private:
    void toggle_run();
    void reset_run();
    void advance();
    void refresh();
    void export_csv();
    bool save_csv(const QString& path) const;

    TelemetryModel model_;
    QTimer* timer_;
    QSpinBox* target_;
    QDoubleSpinBox* gain_;
    QPushButton* start_;
    QPushButton* reset_;
    QPushButton* export_;
    QLabel* altitude_;
    QLabel* error_;
    QLabel* command_;
    QLabel* status_;
    QLabel* progress_;
    TelemetryChart* height_chart_;
    TelemetryChart* control_chart_;
    bool started_ = false;
};
