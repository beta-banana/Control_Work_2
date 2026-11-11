#include "controller.h"

SimpleController::SimpleController(double coefficient) : coefficient_(coefficient) {}
double SimpleController::calculate_control(double error, double)
{
    return coefficient_ * error;
}
QString SimpleController::controller_name() const { return "Simple"; }
