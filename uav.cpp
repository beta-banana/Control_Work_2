#include "uav.h"
#include <QDebug>

void Multicopter::takeoff() { qDebug().noquote() << type() + ": " + takeoff_type(); }
void Multicopter::land() { qDebug().noquote() << type() + ": " + landing_type(); }
QString Multicopter::takeoff_type() const { return "vertical takeoff"; }
QString Multicopter::landing_type() const { return "vertical landing"; }
QString Quadcopter::type() const { return "Quadcopter"; }
int Quadcopter::motor_count() const { return 4; }
QString Hexacopter::type() const { return "Hexacopter"; }
int Hexacopter::motor_count() const { return 6; }
QString Octocopter::type() const { return "Octocopter"; }
int Octocopter::motor_count() const { return 8; }

void print_vehicle_info(const UAV& vehicle)
{
    qDebug().noquote() << QString("%1: %2 motors").arg(vehicle.type()).arg(vehicle.motor_count());
    qDebug().noquote() << "Takeoff:" << vehicle.takeoff_type();
    qDebug().noquote() << "Landing:" << vehicle.landing_type();
}
