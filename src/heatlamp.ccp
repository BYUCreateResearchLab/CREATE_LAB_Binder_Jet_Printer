#include "heatlamp.h"

HeatLamp::HeatLamp(double target_temp, QObject *parent):
    QObject(parent),
    target_temp(target_temp)
{
    this.target_temp = target_temp;
}

HeatLamp::~HeatLamp()
{

}

HeatLamp::get_next_voltage() {
    if(target_temp > temp_history.back().temp) {
        last_voltage = min(max(0, temp_history.back().voltage + 0.2), max_voltage);
    } else {
        last_voltage = min(max(0, temp_history.back().voltage - 0.2), max_voltage);
    }
    return last_voltage;
}

HeatLamp::set_last_temp(double temperature) {
    TempData data;
    data.voltage = last_voltage;
    data.temp = temperature;
    temp_history.push_back(data);
}