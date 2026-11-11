#pragma once
#include <QString>
#include <vector>

class Sensor {
public:
    virtual ~Sensor() = default;
    virtual double read() = 0;
    virtual bool is_ready() const = 0;
    virtual QString name() const = 0;
    virtual double accuracy() const = 0;
    virtual QString reading_text() = 0;
    virtual void set_ready(bool ready) = 0;
};

class NavigationSensor : public Sensor {
public:
    bool is_ready() const override;
    void set_ready(bool ready) override;
private:
    bool ready_ = true;
};

class GPS final : public NavigationSensor {
public:
    double read() override;
    QString name() const override;
    double accuracy() const override;
    QString reading_text() override;
};
class GLONASS final : public NavigationSensor {
public:
    double read() override;
    QString name() const override;
    double accuracy() const override;
    QString reading_text() override;
};
class VisualOdometry final : public NavigationSensor {
public:
    double read() override;
    QString name() const override;
    double accuracy() const override;
    QString reading_text() override;
};

Sensor* choose_best_sensor(const std::vector<Sensor*>& sensors);
