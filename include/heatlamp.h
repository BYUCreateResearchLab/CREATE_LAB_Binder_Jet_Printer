#ifndef HEATLAMP_H
#define HEATLAMP_H

#include <vector>
#include <QObject>

struct TempData {
    double voltage;
    double temp;
};

class HeatLamp : public QObject
{
    Q_OBJECT

    public:
        explicit HeatLamp(double target_temp, QObject *parent = nullptr);
        ~HeatLamp();
        double get_next_voltage();
        void set_last_temp(double temperature);
        double target_temp;
        double kp{0.1}; // volts per degree
        double ki{0.05}; //volts per degree*passes
        void clear_history();
    
    private:
        double last_voltage;
        std::vector<TempData> temp_history;
        double max_voltage{10.0};
        double min_voltage{-10.0};
};

#endif
