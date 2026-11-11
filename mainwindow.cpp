#include "mainwindow.h"
#include "instruments.h"
#include "telemetrypage.h"
#include "controller.h"
#include "debuglog.h"

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStyle>
#include <QTextCursor>
#include <QVBoxLayout>
#include <algorithm>

namespace {
const std::array<QString, 5> titles{"Летательные аппараты", "Навигационные сенсоры",
                                  "Резервирование навигации", "Цикл автопилота", "Телеметрия"};
const std::array<QString, 5> descriptions{
    "Сравните три конфигурации мультикоптеров и проведите лётное испытание.",
    "Отключайте источники и наблюдайте, как система выбирает самый точный из готовых.",
    "Отключайте системы по очереди, чтобы увидеть автоматический переход на резерв.",
    "Запустите расчёт управления по высоте и проверьте реакцию системы на отказы.",
    "Задайте высоту и посмотрите, как регулятор ведёт аппарат к цели."
};
const std::array<QString, 3> aircraftNames{"Квадрокоптер", "Гексакоптер", "Октокоптер"};
const std::array<QString, 3> sensorNames{"GPS", "ГЛОНАСС", "Визуальная одометрия"};
const std::array<QString, 3> navNames{"GPS", "INS", "Визуальная система"};

QLabel* label(const QString& text, const char* name = "")
{
    auto* result = new QLabel(text);
    result->setObjectName(name);
    result->setTextFormat(Qt::PlainText);
    return result;
}
QLabel* note(const QString& text)
{
    auto* result = label(text, "instruction");
    result->setWordWrap(true);
    return result;
}
QPushButton* button(const QString& title, const char* role)
{
    auto* result = new QPushButton(title);
    result->setProperty("role", role);
    result->setCursor(Qt::PointingHandCursor);
    return result;
}
QVBoxLayout* add_page(QStackedWidget* stack, const QString& title, const QString& instruction)
{
    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    layout->addWidget(label(title, "sectionTitle"));
    layout->addWidget(note(instruction));
    stack->addWidget(widget);
    return layout;
}
QString error_description(const QString& error)
{
    if (error == "sensor failure") return "Сенсор не готов";
    if (error == "Navigation data invalid") return "Навигация недоступна";
    if (error == "Error: flight controller is not set") return "Регулятор не подключён";
    if (error == "Error: invalid navigation position") return "Некорректная позиция";
    if (error == "Error: vehicle is not set") return "Аппарат не подключён";
    return error;
}
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Контрольная работа № 2");
    setMinimumSize(1040, 694);
    resize(1220, 790);
    QFile css(":/ui/style.qss");
    if (css.open(QIODevice::ReadOnly)) setStyleSheet(QString::fromUtf8(css.readAll()));

    auto* root = new QWidget;
    root->setObjectName("root");
    setCentralWidget(root);
    auto* overall = new QHBoxLayout(root);
    overall->setContentsMargins(0, 0, 0, 0);
    overall->setSpacing(0);

    auto* rail = new QFrame;
    rail->setObjectName("rail");
    rail->setFixedWidth(208);
    overall->addWidget(rail);
    auto* railLayout = new QVBoxLayout(rail);
    railLayout->setContentsMargins(19, 25, 19, 25);
    railLayout->setSpacing(9);
    auto* mark = label("02", "mark");
    mark->setFixedSize(45, 43);
    mark->setAlignment(Qt::AlignCenter);
    railLayout->addWidget(mark);
    railLayout->addWidget(label("Контрольная работа № 2", "railNote"));
    railLayout->addSpacing(27);
    railLayout->addWidget(label("Разделы работы", "railCaption"));

    routes_ = new QButtonGroup(this);
    routes_->setExclusive(true);
    const std::array<QString, 5> routes{"01   Аппараты", "02   Сенсоры",
                                       "03   Навигация", "04   Автопилот", "05   Телеметрия"};
    for (int i = 0; i < int(routes.size()); ++i) {
        auto* route = button(routes[i], "route");
        route->setCheckable(true);
        route->setMinimumHeight(42);
        routes_->addButton(route, i);
        railLayout->addWidget(route);
    }
    railLayout->addStretch();
    railLayout->addWidget(label("Учебная симуляция", "railNote"));

    auto* area = new QWidget;
    overall->addWidget(area, 1);
    auto* workspace = new QVBoxLayout(area);
    workspace->setContentsMargins(26, 20, 26, 22);
    workspace->setSpacing(12);
    auto* top = new QHBoxLayout;
    top->addWidget(label("ПО объектов наземной инфраструктуры", "context"));
    top->addStretch();
    top->addWidget(label("Вариант 2", "variant"));
    workspace->addLayout(top);
    auto* heading = new QVBoxLayout;
    heading->setSpacing(3);
    title_ = label({}, "pageTitle");
    description_ = label({}, "pageDescription");
    description_->setWordWrap(true);
    heading->addWidget(title_);
    heading->addWidget(description_);
    workspace->addLayout(heading);

    exercise_ = new QWidget;
    auto* body = new QHBoxLayout(exercise_);
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(14);
    instruments_ = new Instruments;
    body->addWidget(instruments_, 1);
    auto* control = new QFrame;
    control->setObjectName("control");
    control->setFixedWidth(267);
    auto* controlLayout = new QVBoxLayout(control);
    controlLayout->setContentsMargins(16, 16, 16, 16);
    controlLayout->setSpacing(8);
    controls_ = new QStackedWidget;
    controlLayout->addWidget(controls_, 1);
    outcome_ = label({}, "outcome");
    outcome_->setWordWrap(true);
    controlLayout->addWidget(outcome_);
    body->addWidget(control);
    workspace->addWidget(exercise_, 1);
    telemetry_ = new TelemetryPage;
    workspace->addWidget(telemetry_, 1);

    auto* vehiclePage = add_page(controls_, "Лётное испытание",
                                  "Выберите аппарат, чтобы посмотреть его схему и характеристики.");
    aircraft_ = new QComboBox;
    aircraft_->setAccessibleName("Тип летательного аппарата");
    for (const auto& name : aircraftNames) aircraft_->addItem(name);
    vehiclePage->addWidget(aircraft_);
    flight_button_ = button("Выполнить испытание", "primary");
    vehiclePage->addWidget(flight_button_);
    vehiclePage->addStretch();

    auto* sensorPage = add_page(controls_, "Готовность сенсоров",
                                "Снимите отметку, чтобы исключить источник из сравнения.");
    for (int i = 0; i < 3; ++i) {
        sensor_ready_[i] = new QCheckBox(sensorNames[i]);
        sensor_ready_[i]->setChecked(true);
        sensorPage->addWidget(sensor_ready_[i]);
        connect(sensor_ready_[i], &QCheckBox::toggled, this, &MainWindow::refresh_sensors);
    }
    auto* restoreSensors = button("Восстановить сенсоры", "secondary");
    sensorPage->addWidget(restoreSensors);
    sensorPage->addStretch();

    auto* navPage = add_page(controls_, "Доступность систем",
                             "Первая доступная система в цепочке становится активной.");
    for (int i = 0; i < 3; ++i) {
        nav_valid_[i] = new QCheckBox(navNames[i]);
        nav_valid_[i]->setChecked(true);
        navPage->addWidget(nav_valid_[i]);
        connect(nav_valid_[i], &QCheckBox::toggled, this, &MainWindow::refresh_navigation);
    }
    auto* restoreNavigation = button("Восстановить системы", "secondary");
    navPage->addWidget(restoreNavigation);
    navPage->addStretch();

    auto* cyclePage = add_page(controls_, "Сценарий испытания",
                               "Каждый запуск начинается с условий выбранного сценария.");
    scenario_ = new QComboBox;
    scenario_->setAccessibleName("Сценарий автопилота");
    scenario_->addItems({"01  Штатный цикл", "02  Сенсор не готов", "03  Нет навигации",
                         "04  Нет регулятора", "05  Высота −10 м"});
    cyclePage->addWidget(scenario_);
    cycle_button_ = button("Запустить цикл", "primary");
    cyclePage->addWidget(cycle_button_);
    cyclePage->addStretch();

    journal_panel_ = new QFrame;
    journal_panel_->setObjectName("journalPanel");
    auto* journalLayout = new QVBoxLayout(journal_panel_);
    journalLayout->setContentsMargins(14, 11, 14, 12);
    journalLayout->setSpacing(7);
    auto* journalHeader = new QHBoxLayout;
    journalHeader->addWidget(label("Журнал испытания", "sectionTitle"));
    journalHeader->addStretch();
    journalHeader->addWidget(label("qDebug()", "logTag"));
    journalLayout->addLayout(journalHeader);
    journal_ = new QPlainTextEdit;
    journal_->setAccessibleName("Журнал испытания");
    journal_->setReadOnly(true);
    journal_->setFixedHeight(128);
    journal_->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPixelSize(12);
    journal_->setFont(font);
    journalLayout->addWidget(journal_);
    workspace->addWidget(journal_panel_);

    connect(routes_, &QButtonGroup::idClicked, this, &MainWindow::switch_page);
    connect(aircraft_, &QComboBox::currentIndexChanged, this, [this] { refresh_airframe(); });
    connect(flight_button_, &QPushButton::clicked, this, [this] { refresh_airframe(true); });
    connect(restoreSensors, &QPushButton::clicked, this, [this] {
        for (auto* checkbox : sensor_ready_) {
            const QSignalBlocker blocked(checkbox);
            checkbox->setChecked(true);
        }
        refresh_sensors();
    });
    connect(restoreNavigation, &QPushButton::clicked, this, [this] {
        for (auto* checkbox : nav_valid_) {
            const QSignalBlocker blocked(checkbox);
            checkbox->setChecked(true);
        }
        refresh_navigation();
    });
    connect(scenario_, &QComboBox::currentIndexChanged, this, &MainWindow::prepare_cycle);
    connect(cycle_button_, &QPushButton::clicked, this, &MainWindow::run_cycle);
    switch_page(Airframe);
}

void MainWindow::switch_page(int page)
{
    routes_->button(page)->setChecked(true);
    exercise_->setVisible(page != Telemetry);
    telemetry_->setVisible(page == Telemetry);
    journal_panel_->setVisible(page != Telemetry);
    if (page != Telemetry) controls_->setCurrentIndex(page);
    title_->setText(titles[page]);
    description_->setText(descriptions[page]);
    switch (page) {
        case Airframe: refresh_airframe(); break;
        case Sensors: refresh_sensors(); break;
        case Navigation: refresh_navigation(); break;
        case Cycle:
            if (cycle_has_run_) {
                instruments_->show_cycle(cycle_result_, true);
                update_journal(cycle_log_);
                show_outcome(cycle_result_.completed ? "Управление рассчитано"
                             : error_description(cycle_result_.error),
                             cycle_result_.completed ? "ok" : "error");
            } else prepare_cycle();
            break;
        case Telemetry: break;
    }
}

void MainWindow::update_journal(const QString& text)
{
    journal_->setPlainText(text);
    journal_->moveCursor(QTextCursor::End);
}

void MainWindow::show_outcome(const QString& message, const char* tone)
{
    outcome_->setText(message);
    outcome_->setProperty("tone", tone);
    outcome_->style()->unpolish(outcome_);
    outcome_->style()->polish(outcome_);
}

void MainWindow::refresh_airframe(bool test)
{
    DebugLog log;
    const int index = aircraft_->currentIndex();
    UAV& vehicle = *fleet_[index];
    print_vehicle_info(vehicle);
    if (test) {
        vehicle.takeoff();
        vehicle.land();
    }
    instruments_->show_airframe(vehicle.motor_count(), aircraftNames[index]);
    show_outcome(test ? "Взлёт и посадка выполнены" : "Готов к испытанию", test ? "ok" : "neutral");
    update_journal(log.text());
}

void MainWindow::refresh_sensors()
{
    DebugLog log;
    std::array<bool, 3> ready{};
    std::array<double, 3> errors{};
    for (size_t i = 0; i < sensors_.size(); ++i) {
        ready[i] = sensor_ready_[i]->isChecked();
        sensors_[i]->set_ready(ready[i]);
        errors[i] = sensors_[i]->accuracy();
    }
    const auto* best = choose_best_sensor(sensors_);
    const auto it = std::find(sensors_.begin(), sensors_.end(), best);
    const int selected = it == sensors_.end() ? -1 : int(it - sensors_.begin());
    instruments_->show_sensors(ready, errors, selected);
    show_outcome(selected < 0 ? "Нет готовых сенсоров" : "Лучший источник: " + sensorNames[selected],
                 selected < 0 ? "error" : "ok");
    update_journal(log.text());
}

void MainWindow::refresh_navigation()
{
    DebugLog log;
    gps_nav_.set_valid(nav_valid_[0]->isChecked());
    ins_.set_valid(nav_valid_[1]->isChecked());
    visual_nav_.set_valid(nav_valid_[2]->isChecked());
    std::array<bool, 3> valid{};
    for (size_t i = 0; i < nav_sources_.size(); ++i)
        valid[i] = nav_sources_[i]->is_valid();
    const auto it = std::find(nav_sources_.begin(), nav_sources_.end(), manager_.active_system());
    const int active = it == nav_sources_.end() ? -1 : int(it - nav_sources_.begin());
    QString coords = "Координаты недоступны";
    if (active >= 0) {
        const auto p = manager_.get_position();
        const QLocale ru(QLocale::Russian);
        coords = QString("Широта %1°   ·   Долгота %2°   ·   Высота %3 м")
                 .arg(ru.toString(p.latitude, 'f', 4), ru.toString(p.longitude, 'f', 4),
                      ru.toString(p.altitude, 'f', 0));
    }
    manager_.print_status();
    instruments_->show_navigation(valid, active, coords);
    show_outcome(active < 0 ? "Все системы недоступны" : "Активна: " + navNames[active],
                 active < 0 ? "error" : "ok");
    update_journal(log.text());
}

void MainWindow::prepare_cycle()
{
    cycle_has_run_ = false;
    cycle_result_ = {};
    cycle_log_.clear();
    instruments_->show_cycle(cycle_result_, false);
    show_outcome("Цикл ещё не запущен", "neutral");
    update_journal({});
}

void MainWindow::run_cycle()
{
    DebugLog log;
    // Независимый стенд: настройки соседних разделов не меняют сценарий.
    GPS gps;
    GLONASS glonass;
    VisualOdometry visual;
    GPSNavigation gpsNav;
    InertialNavigation ins;
    VisualNavigation visualNav;
    NavigationManager navigation({&gpsNav, &ins, &visualNav});
    SimpleController simple;
    FlightController* controller = &simple;
    switch (scenario_->currentIndex()) {
        case 1: gps.set_ready(false); break;
        case 2:
            gpsNav.set_valid(false);
            ins.set_valid(false);
            visualNav.set_valid(false);
            break;
        case 3: controller = nullptr; break;
        case 4: gpsNav.set_position({55.75, 37.61, -10}); break;
        default: break;
    }
    Autopilot autopilot(fleet_[aircraft_->currentIndex()], {&gps, &glonass, &visual},
                        &navigation, controller);
    autopilot.run_cycle();
    cycle_result_ = autopilot.last_result();
    cycle_log_ = log.text();
    cycle_has_run_ = true;
    switch_page(Cycle);
}

bool MainWindow::verify_and_capture(const QString& directory)
{
    QDir out(directory);
    if (!out.exists() && !QDir().mkpath(directory)) return false;
    const auto capture = [&](const QString& filename) {
        if (auto* focus = focusWidget()) focus->clearFocus();
        QApplication::processEvents();
        return grab().save(out.filePath(filename));
    };

    routes_->button(Airframe)->click();
    for (int i = 0; i < 3; ++i) {
        aircraft_->setCurrentIndex(i);
        flight_button_->click();
        if (!journal_->toPlainText().contains(QString("%1 motors").arg(4 + 2*i))
            || !journal_->toPlainText().contains("vertical landing")) return false;
    }
    if (!capture("01-airframe.png")) return false;

    routes_->button(Sensors)->click();
    sensor_ready_[0]->click();
    if (!journal_->toPlainText().contains("Best sensor: GLONASS")) return false;
    if (!capture("02-sensors.png")) return false;
    sensor_ready_[1]->click();
    if (!journal_->toPlainText().contains("Best sensor: Visual odometry")) return false;
    sensor_ready_[2]->click();
    if (!journal_->toPlainText().contains("no navigation sensor is ready")) return false;

    routes_->button(Navigation)->click();
    nav_valid_[0]->click();
    if (!journal_->toPlainText().contains("Active navigation system: INS")) return false;
    if (!capture("03-navigation.png")) return false;
    nav_valid_[1]->click();
    if (!journal_->toPlainText().contains("Active navigation system: Visual")) return false;
    nav_valid_[2]->click();
    if (!journal_->toPlainText().contains("Critical error")) return false;

    routes_->button(Cycle)->click();
    const QStringList expected{"Controller output: 25", "Reason: sensor failure",
                               "Navigation data invalid", "flight controller is not set",
                               "invalid navigation position"};
    for (int i = 0; i < expected.size(); ++i) {
        scenario_->setCurrentIndex(i);
        cycle_button_->click();
        if (!journal_->toPlainText().contains(expected[i]) || cycle_result_.completed != (i == 0))
            return false;
        if (i == 0 && !capture("04-autopilot.png")) return false;
        if (i == 2 && !capture("05-autopilot-abort.png")) return false;
    }
    routes_->button(Telemetry)->click();
    if (!telemetry_->verify_interactions() || !capture("06-telemetry.png")) return false;
    switch_page(Airframe);
    return true;
}
