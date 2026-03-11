#include "heatlamp.h"

HeatLamp::HeatLamp(double target_temp, QObject *parent):
    QObject(parent),
    target_temp(target_temp)
{

}

HeatLamp::~HeatLamp()
{

}

void HeatLamp::clear_history() {
    temp_history.clear();
    last_voltage = 0;
}

double HeatLamp::get_next_voltage() {
    // if(target_temp > temp_history.back().temp) {
    //     last_voltage = std::min(std::max(0.0, temp_history.back().voltage + 0.2), max_voltage);
    // } else {
    //     last_voltage = std::min(std::max(0.0, temp_history.back().voltage - 0.2), max_voltage);
    // }
    
    if(temp_history.size() == 0) {
        last_voltage = starting_voltage;
        return last_voltage;
    } else {
        double error_integral = 0;
        for(TempData data : temp_history) {
            error_integral += target_temp - data.temp;
        }
        last_voltage += kp*(target_temp - temp_history.back().temp) + error_integral*ki;
        last_voltage = std::min(std::max(min_voltage, last_voltage), max_voltage);
        return last_voltage;
    }
}

void HeatLamp::set_last_temp(double temperature) {
    TempData data;
    data.voltage = last_voltage;
    data.temp = temperature;
    temp_history.push_back(data);
}
