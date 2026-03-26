#include "heatlamp.h"
#include <QDebug>

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
    last_intensity = 0;
}

std::string HeatLamp::set_intensity(int intensity) {
    std::stringstream ss;
    if (intensity%2 == 0) {
        ss << CMD::set_bit(HEATLAMP_D0);
    } else {
        ss << CMD::clear_bit(HEATLAMP_D0);
    }
    intensity /= 2;
    if (intensity%2 == 0) {
        ss << CMD::set_bit(HEATLAMP_D1);
    } else {
        ss << CMD::clear_bit(HEATLAMP_D1);
    }
    intensity /= 2;
    if (intensity%2 == 0) {
        ss << CMD::set_bit(HEATLAMP_D2);
    } else {
        ss << CMD::clear_bit(HEATLAMP_D2);
    }
    intensity /= 2;
    if (intensity%2 == 0) {
        ss << CMD::set_bit(HEATLAMP_D3);
    } else {
        ss << CMD::clear_bit(HEATLAMP_D3);
    }
    return ss.str();
}

int HeatLamp::get_next_intensity() {
    // if(target_temp > temp_history.back().temp) {
    //     last_voltage = std::min(std::max(0.0, temp_history.back().voltage + 0.2), max_voltage);
    // } else {
    //     last_voltage = std::min(std::max(0.0, temp_history.back().voltage - 0.2), max_voltage);
    // }
    
    if(temp_history.size() == 0) {
        last_intensity = starting_intensity;
        return last_intensity;
    } else {
        double error_integral = 0;
        for(TempData data : temp_history) {
            error_integral += target_temp - data.temp;
        }
        last_intensity += kp*(target_temp - temp_history.back().temp) + error_integral*ki;
        last_intensity = std::min(std::max(min_intensity, last_intensity), max_intensity);
        return last_intensity;
    }
}

void HeatLamp::set_last_temp(double temperature) {
    TempData data;
    data.intensity = last_intensity;
    data.temp = temperature;
    temp_history.push_back(data);
}
