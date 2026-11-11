#pragma once
#include <QWidget>
#include <QPointF>
#include "telemetrymodel.h"

class TelemetryChart final : public QWidget {
public:
    enum Series { Altitude, Control };
    TelemetryChart(const TelemetryModel& model, Series series, QWidget* parent = nullptr);
protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
private:
    const TelemetryModel& model_;
    Series series_;
    QPointF pointer_{-1, -1};
};
