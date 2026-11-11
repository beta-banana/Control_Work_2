#pragma once
#include <QString>

class FlightController {
public:
    virtual ~FlightController() = default;
    virtual double calculate_control(double error, double dt) = 0;
    virtual QString controller_name() const = 0;
};
class SimpleController final : public FlightController {
public:
    explicit SimpleController(double coefficient = 0.5);
    double calculate_control(double error, double dt) override;
    QString controller_name() const override;
private:
    double coefficient_;
};
