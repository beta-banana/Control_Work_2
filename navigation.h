#pragma once
#include <QString>
#include <vector>

struct Position {
    double latitude;
    double longitude;
    double altitude;
};

class NavigationSystem {
public:
    virtual ~NavigationSystem() = default;
    virtual Position get_position() const = 0;
    virtual bool is_valid() const = 0;
    virtual QString system_name() const = 0;
};
class GPSNavigation final : public NavigationSystem {
public:
    Position get_position() const override;
    bool is_valid() const override;
    QString system_name() const override;
    void set_valid(bool valid);
    void set_satellites(int count);
    void set_position(Position position);
private:
    Position position_{55.75, 37.61, 150};
    int satellites_ = 10;
    bool valid_ = true;
};
class InertialNavigation final : public NavigationSystem {
public:
    Position get_position() const override;
    bool is_valid() const override;
    QString system_name() const override;
    void set_valid(bool valid);
private:
    Position position_{55.7502, 37.6102, 148};
    bool valid_ = true;
};
class VisualNavigation final : public NavigationSystem {
public:
    Position get_position() const override;
    bool is_valid() const override;
    QString system_name() const override;
    void set_valid(bool tracked);
private:
    Position position_{55.7504, 37.6103, 152};
    bool target_tracked_ = true;
};

// Менеджер также реализует NavigationSystem, поэтому автопилот видит его
// как обычную навигацию и получает актуальный резервный источник.
class NavigationManager final : public NavigationSystem {
public:
    explicit NavigationManager(const std::vector<NavigationSystem*>& sources = {});
    void set_system(NavigationSystem* system);
    bool is_available() const;
    const NavigationSystem* active_system() const;
    Position get_position() const override;
    bool is_valid() const override;
    QString system_name() const override;
    void print_status() const;
private:
    std::vector<NavigationSystem*> sources_;
    mutable NavigationSystem* active_system_ = nullptr;
};
