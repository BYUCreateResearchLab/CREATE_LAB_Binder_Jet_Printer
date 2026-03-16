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
        double kp{0.05}; // volts per degree
        double ki{0.02}; //volts per degree*passes
        void clear_history();
        double max_voltage{-8.4};
        double min_voltage{-8.9};
        double starting_voltage {-8.5};
    
    private:
        double last_voltage;
        std::vector<TempData> temp_history;
};

#endif
