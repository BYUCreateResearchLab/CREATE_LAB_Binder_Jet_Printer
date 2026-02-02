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
        double set_last_temp();
        double target_temp;
        int pin_address = 1; //TODO update pin_address 
    
    private:
        double last_voltage;
        std::vector<TempData> temp_history;
        double max_voltage;
};

#endif
