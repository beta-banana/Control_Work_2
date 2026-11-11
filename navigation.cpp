#include "navigation.h"
#include <QDebug>
#include <limits>

Position GPSNavigation::get_position() const { return position_; }
bool GPSNavigation::is_valid() const { return valid_ && satellites_ >= 4; }
QString GPSNavigation::system_name() const { return "GPS"; }
void GPSNavigation::set_valid(bool valid) { valid_ = valid; }
void GPSNavigation::set_satellites(int count) { satellites_ = count; }
void GPSNavigation::set_position(Position position) { position_ = position; }

Position InertialNavigation::get_position() const { return position_; }
bool InertialNavigation::is_valid() const { return valid_; }
QString InertialNavigation::system_name() const { return "INS"; }
void InertialNavigation::set_valid(bool valid) { valid_ = valid; }

Position VisualNavigation::get_position() const { return position_; }
bool VisualNavigation::is_valid() const { return target_tracked_; }
QString VisualNavigation::system_name() const { return "Visual"; }
void VisualNavigation::set_valid(bool tracked) { target_tracked_ = tracked; }

NavigationManager::NavigationManager(const std::vector<NavigationSystem*>& sources)
    : sources_(sources) {}

void NavigationManager::set_system(NavigationSystem* system)
{
    sources_ = {system};
    active_system_ = nullptr;
}

const NavigationSystem* NavigationManager::active_system() const
{
    active_system_ = nullptr;
    // Порядок источников задаётся при сборке системы, без проверок их типов.
    for (auto* system : sources_) {
        if (system && system->is_valid()) {
            active_system_ = system;
            break;
        }
    }
    return active_system_;
}

bool NavigationManager::is_available() const { return active_system() != nullptr; }
bool NavigationManager::is_valid() const { return is_available(); }

Position NavigationManager::get_position() const
{
    if (auto* system = active_system()) return system->get_position();
    const double missing = std::numeric_limits<double>::quiet_NaN();
    return {missing, missing, missing};
}

QString NavigationManager::system_name() const
{
    if (auto* system = active_system()) return system->system_name();
    return "Unavailable";
}

void NavigationManager::print_status() const
{
    for (auto* system : sources_) {
        if (system)
            qDebug().noquote() << system->system_name() + ":" << (system->is_valid() ? "valid" : "invalid");
    }
    if (auto* system = active_system()) {
        qDebug().noquote() << "Active navigation system:" << system->system_name();
        const auto p = system->get_position();
        qDebug().noquote() << QString("Position: %1, %2, %3 m")
                              .arg(p.latitude, 0, 'f', 4).arg(p.longitude, 0, 'f', 4).arg(p.altitude);
    } else {
        qDebug() << "Critical error: no navigation system is available";
    }
}
